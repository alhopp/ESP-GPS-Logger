#include "Ublox.h"
#include "Definitions.h"
#include <EEPROM.h>
#include "config_manager.h"
#include "rtc_state.h"

UBXMessage ubxMessage = {};

// ============================================================================
// u-blox UART interface
//
// Owns the dedicated ESP32 UART used to communicate with the u-blox M10.
//
// - Uses HardwareSerial port 2 (UART2)
// - Default baud rate: 38400
// - UBX-only protocol (NMEA disabled during init)
// - RX/TX pins defined in Definitions.h
// ============================================================================

HardwareSerial UbloxSerial(2);

void ubloxSerialInit(int delay_ms)
{
    UbloxSerial.begin(
        38400,
        SERIAL_8N1,
        GPS_UART_RX_PIN,
        GPS_UART_TX_PIN
    );
    delay(delay_ms);   
}


// -----------------------------------------------------------------------------
// Set_GPS_Time
//
// Sets the ESP32 system clock using UTC time from the u-blox NAV-PVT message.
//
// Returns:
// - true  → system time successfully set from GPS
// - false → GPS time invalid or update failed
// -----------------------------------------------------------------------------

bool Set_GPS_Time(float time_offset)
{
    const auto& pvt = ubxMessage.navPvt;

    // Require sane GPS time
    if (pvt.year < 2023) return false;

    tm t{};
    t.tm_sec  = pvt.second;
    t.tm_min  = pvt.minute;
    t.tm_hour = pvt.hour + (int)time_offset;
    t.tm_mday = pvt.day;
    t.tm_mon  = pvt.month - 1;        // 0–11
    t.tm_year = pvt.year - 1900;      // since 1900

    setenv("TZ", "UTC0", 1);
    tzset();

    time_t utc = mktime(&t);
    if (utc < 1672531200) return false; // < 2023-01-01

    timeval tv{ utc, 0 };
    settimeofday(&tv, nullptr);

    setenv("TZ", TimeZone, 1);
    tzset();

    tm verify{};
    if (!getLocalTime(&verify)) return false;
    if (verify.tm_year + 1900 < 2023) return false;

    Serial.println("GPS Local Time set");
    return true;
}

// ============================================================================
// CFG-PRT (UART1) — Board-specific UART configuration for u-blox M10
//
// Purpose:
//   • Forces UART1 into UBX-only mode (no NMEA)
//   • Enables UBX input + output
//   • Sets UART framing to 8N1
//   • Establishes a known baud rate (38400)
//   • UART1 refers to the GPS module's UART, not ESP32 UART numbering
// ============================================================================
namespace ubx::cfg {

  const uint8_t uart1_ubx_in_out[] PROGMEM = {
    0xB5,0x62,            // UBX sync chars
    0x06,0x00,            // CFG-PRT
    0x14,0x00,            // payload length (20 bytes)

    0x01,                 // portID = UART1
    0x00,                 // reserved
    0x00,0x00,            // txReady

    0xD0,0x08,0x00,0x00,  // UART mode = 8N1
    0x00,0x96,0x00,0x00,  // baud rate = 38400

    0x01,0x00,            // inProtoMask  = UBX
    0x01,0x00,            // outProtoMask = UBX

    0x00,0x00,            // flags
    0x00,0x00,            // reserved

    0xA2,0xB5             // checksum (CK_A, CK_B)
  };

}

// ============================================================================
// Init_ubloxM10
//
// Fully initialise a u-blox M10 GNSS receiver into a deterministic runtime
// configuration suitable for high-rate UBX-only operation.
//
// Guarantees on return:
//   • UART1 configured for UBX IN + OUT (no NMEA)
//   • GPS + GLONASS + GALILEO + BEIDOU(B1C) enabled
//   • Motion model set to SEA
//   • Navigation rate fixed at 5 Hz
//   • NAV-PVT and NAV-DOP enabled (NAV-SAT optional)
//   • Diagnostic metadata polled (MON-VER, MON-GNSS, UID)
//   • ESP32 UART restarted and synchronised at 38400 baud
//
// Notes:
//   • Commands are deliberately sequenced with delays to match M10 behaviour
//   • UBX IN+OUT is asserted twice (before and after baud switch) for safety
//   • This function assumes UbloxSerial pins are already configured
// ============================================================================
void Init_ubloxM10(void)
{
    constexpr uint16_t WAIT_MS = 200;

    LOG_GPS("Init", "===== u-blox M10 INIT START =====");

    // ---------------------------------------------------------------------
    // 1. Force UART1 into UBX-only mode (critical first step)
    // ---------------------------------------------------------------------
    LOG_GPS("CFG", "UART1: UBX IN+OUT");
    sendUbx(ubx::cfg::uart1_ubx_in_out);
    delay(WAIT_MS);

    // ---------------------------------------------------------------------
    // 2. Disable NMEA output (binary-only operation)
    // ---------------------------------------------------------------------
    LOG_GPS("CFG", "Disable NMEA");
    sendUbx(ubx::cfg::nmea_off);
    delay(WAIT_MS);

    // ---------------------------------------------------------------------
    // 3. Enable all supported GNSS constellations
    // ---------------------------------------------------------------------
    LOG_GPS("CFG", "GNSS: GPS + GAL + GLO + BDS(B1C)");
    sendUbx(ubx::cfg::all_4gnss);
    delay(WAIT_MS);

    // ---------------------------------------------------------------------
    // 4. Set dynamic motion model
    // ---------------------------------------------------------------------
    LOG_GPS("CFG", "Motion model: SEA");
    sendUbx(ubx::cfg::sea_model);
    delay(WAIT_MS);

    // ---------------------------------------------------------------------
    // 5. Fix navigation output rate (5 Hz)
    // ---------------------------------------------------------------------
    LOG_GPS("CFG", "Nav rate: 5 Hz");
    sendUbx(ubx::rate::rate_5hz);
    delay(WAIT_MS);

    // ---------------------------------------------------------------------
    // 6. Enable required navigation messages
    // ---------------------------------------------------------------------
    LOG_GPS("MSG", "Enable NAV-PVT");
    sendUbx(ubx::msg::nav_pvt);
    delay(WAIT_MS);

    LOG_GPS("MSG", "Enable NAV-DOP");
    sendUbx(ubx::msg::nav_dop);
    delay(WAIT_MS);

    if (config.logUBX && config.logUBX_nav_sat) {
        LOG_GPS("MSG", "Enable NAV-SAT");
        sendUbx(ubx::msg::nav_sat);
        delay(WAIT_MS);
    }

    // ---------------------------------------------------------------------
    // 7. Poll diagnostic / identity information
    // ---------------------------------------------------------------------
    LOG_GPS("POLL", "MON-VER");
    sendUbx(ubx::poll::mon_ver);
    delay(WAIT_MS);

    LOG_GPS("POLL", "MON-GNSS");
    sendUbx(ubx::poll::mon_gnss);
    delay(WAIT_MS);

    LOG_GPS("POLL", "UID");
    sendUbx(ubx::poll::uid);
    delay(WAIT_MS);

    // ---------------------------------------------------------------------
    // 8. Switch GPS UART baud rate to 38400
    // ---------------------------------------------------------------------
    LOG_GPS("CFG", "Switch GPS baud → 38400");
    sendUbx(ubx::rate::baud_38400);
    delay(WAIT_MS);

    // ---------------------------------------------------------------------
    // 9. Restart ESP32 UART to match GPS baud
    // ---------------------------------------------------------------------
    LOG_GPS("UART", "Restart ESP UART @38400");
    UbloxSerial.flush();
    UbloxSerial.end();
    delay(50);

    UbloxSerial.begin(
        38400,
        SERIAL_8N1,
        GPS_UART_RX_PIN,
        GPS_UART_TX_PIN
    );
    delay(200);

    // ---------------------------------------------------------------------
    // 10. Re-assert UBX IN+OUT after baud change (belt & braces)
    // ---------------------------------------------------------------------
    LOG_GPS("CFG", "Re-assert UART1 UBX IN+OUT @38400");
    sendUbx(ubx::cfg::uart1_ubx_in_out);
    delay(WAIT_MS);

    LOG_GPS("Init", "===== u-blox M10 INIT COMPLETE =====");
}

// -----------------------------------------------------------------------------
// classifyMessage
//
// Map UBX class + ID to internal message type.
// Kept inline so the compiler can fold it directly into processGPS()
// with zero call overhead.
// -----------------------------------------------------------------------------
inline uint8_t classifyMessage(uint8_t cls, uint8_t id)
  {
      switch (cls) {
          case 0x01: // NAV
              if (id == 0x07) return MT_NAV_PVT;
              if (id == 0x04) return MT_NAV_DOP;
              if (id == 0x35) return MT_NAV_SAT;
              break;
      }
      return MT_NONE;
  }


  // -----------------------------------------------------------------------------
// Helper: initialise UBX header fields for messages that embed cls/id/len
// C++11-safe (no generic lambdas)
// -----------------------------------------------------------------------------
template <typename T>
inline void initUbxHeader(T &m, uint8_t cls, uint8_t id, uint16_t len)
{
    m.cls = cls;
    m.id  = id;
    m.len = len;
}


// -----------------------------------------------------------------------------
// processGPS()
//
// Stream parser for u-blox UBX frames coming in on UbloxSerial.
//
// UBX frame format:
//   0xB5 0x62 | CLS ID | LEN_L LEN_H | PAYLOAD[len] | CK_A CK_B
//
// Returns:
//   One of MT_* when a full valid frame is decoded,
//   or MT_NONE when no complete valid frame is available yet.
//
// Notes:
// - Parser state is static (persists across calls)
// - Payload is streamed directly into ubxMessage structs
// - Unknown messages are copied (up to 256 bytes) into scratch[]
// -----------------------------------------------------------------------------
int processGPS()
{
    enum State : uint8_t {
        S_SYNC1 = 0,
        S_SYNC2,
        S_CLS,
        S_ID,
        S_LEN1,
        S_LEN2,
        S_PAYLOAD,
        S_CK_A,
        S_CK_B
    };

    // -------------------------------------------------------------------------
    // Persistent UBX parser state
    // -------------------------------------------------------------------------
    static State    st = S_SYNC1;
    static uint8_t  cls = 0, id = 0;
    static uint16_t len = 0, payPos = 0;
    static uint8_t  ckA = 0, ckB = 0;
    static uint8_t  currentMsgType = MT_NONE;

    // -------------------------------------------------------------------------
    // Small helpers (lambdas are fine – no generic params)
    // -------------------------------------------------------------------------

    // Reset parser to sync state
    auto resetFrame = [&]() {
        st = S_SYNC1;
        cls = id = 0;
        len = payPos = 0;
        ckA = ckB = 0;
    };

    // Start checksum accumulation
    auto beginChecksum = [&]() { ckA = ckB = 0; };

    // UBX checksum update (Fletcher-8)
    auto addChecksum = [&](uint8_t b) { ckA += b; ckB += ckA; };

    // -------------------------------------------------------------------------
    // NAV-SAT payload handler (header + repeating satellite blocks)
    // -------------------------------------------------------------------------
    auto handleNavSatPayload = [&](uint8_t c)
    {
        constexpr uint16_t UBX_HDR = 4;

        if (payPos == 0)
            initUbxHeader(ubxMessage.navSatHdr, cls, id, len);

        const uint16_t hdrCap = (uint16_t)(sizeof(NAV_SAT_HDR) - UBX_HDR);

        if (payPos < hdrCap) {
            ((uint8_t*)&ubxMessage.navSatHdr)[UBX_HDR + payPos] = c;
            return;
        }

        const uint16_t satOfs = (uint16_t)(payPos - hdrCap);
        if (satOfs < sizeof(ubxMessage.navSat)) {
            ((uint8_t*)ubxMessage.navSat)[satOfs] = c;
        }
    };

    // -------------------------------------------------------------------------
    // Payload byte handler (NAV-only runtime mode)
    //
    // Keeps only:
    //   - NAV-PVT  : position, velocity, Doppler speed, accuracy
    //   - NAV-DOP  : DOP / quality metrics
    //   - NAV-SAT  : satellite SNR / bars (optional)
    //
    // All MON / ACK / ID handling removed.
    // -------------------------------------------------------------------------
    auto handlePayloadByte = [&](uint8_t c)
    {
        constexpr uint16_t UBX_HDR = 4;

        switch (currentMsgType) {

            // -------------------------------------------------------------
            // NAV-PVT: payload-only struct (no cls/id/len fields)
            // -------------------------------------------------------------
            case MT_NAV_PVT:
                if (payPos < sizeof(NAV_PVT))
                    ((uint8_t*)&ubxMessage.navPvt)[payPos] = c;
                break;

            // -------------------------------------------------------------
            // NAV-DOP: includes cls/id/len, payload starts at +4
            // -------------------------------------------------------------
            case MT_NAV_DOP:
                if (payPos == 0)
                    initUbxHeader(ubxMessage.navDOP, cls, id, len);
                if (payPos < sizeof(NAV_DOP) - UBX_HDR)
                    ((uint8_t*)&ubxMessage.navDOP)[UBX_HDR + payPos] = c;
                break;

            // -------------------------------------------------------------
            // NAV-SAT: satellite info (optional, UI-only)
            // -------------------------------------------------------------
            case MT_NAV_SAT:
                handleNavSatPayload(c);
                break;

            // -------------------------------------------------------------
            // Everything else is ignored
            // -------------------------------------------------------------
            default:
                break;
        }
    };


    // -------------------------------------------------------------------------
    // Main byte-wise UBX state machine
    //
    // Consumes raw bytes from UbloxSerial and reconstructs UBX frames
    // incrementally using a finite-state parser.
    //
    // UBX frame format:
    //   B5 62 | CLS | ID | LEN_L | LEN_H | PAYLOAD[len] | CK_A | CK_B
    //
    // The parser is fully streaming:
    //  - robust to noise / partial frames
    //  - resynchronises automatically
    //  - validates checksum before accepting a message
    // -------------------------------------------------------------------------
    while (UbloxSerial.available()) {
        const uint8_t c = UbloxSerial.read();

        switch (st) {

            // -------------------------------------------------------------
            // SYNC1: wait for first UBX sync byte (0xB5)
            // -------------------------------------------------------------
            case S_SYNC1:
                if (c == 0xB5) st = S_SYNC2;
                break;

            // -------------------------------------------------------------
            // SYNC2: wait for second UBX sync byte (0x62)
            //  - if another 0xB5 arrives, stay here (fast re-sync)
            //  - otherwise fall back to SYNC1
            // -------------------------------------------------------------
            case S_SYNC2:
                st = (c == 0x62) ? S_CLS : (c == 0xB5 ? S_SYNC2 : S_SYNC1);
                break;

            // -------------------------------------------------------------
            // CLS: message class (e.g. NAV, MON, CFG)
            //  - checksum starts here (sync bytes are excluded)
            // -------------------------------------------------------------
            case S_CLS:
                cls = c; 
                beginChecksum();
                addChecksum(c);
                st = S_ID;
                break;

            // -------------------------------------------------------------
            // ID: message ID within class (e.g. NAV-PVT, NAV-SAT)
            // -------------------------------------------------------------
            case S_ID:
                id = c;
                addChecksum(c);
                st = S_LEN1;
                break;

            // -------------------------------------------------------------
            // LEN1: payload length (LSB)
            // -------------------------------------------------------------
            case S_LEN1:
                len = c;
                addChecksum(c);
                st = S_LEN2;
                break;

            // -------------------------------------------------------------
            // LEN2: payload length (MSB)
            //  - full payload length now known
            //  - classify message type once (avoids repeated branching)
            // -------------------------------------------------------------
            case S_LEN2:
                len |= (uint16_t)c << 8;
                addChecksum(c);
                payPos = 0;
                currentMsgType = classifyMessage(cls, id);
                st = (len == 0) ? S_CK_A : S_PAYLOAD;
                break;

            // -------------------------------------------------------------
            // PAYLOAD: stream payload bytes directly into target structs
            // -------------------------------------------------------------
            case S_PAYLOAD:
                handlePayloadByte(c);
                addChecksum(c);
                if (++payPos >= len) st = S_CK_A;
                break;

            // -------------------------------------------------------------
            // CK_A: verify first checksum byte
            // -------------------------------------------------------------
            case S_CK_A:
                if (c != ckA) resetFrame();
                else st = S_CK_B;
                break;

            // -------------------------------------------------------------
            // CK_B: verify second checksum byte
            //  - if valid, finalise message and return its type
            //  - parser is reset ready for next frame
            // -------------------------------------------------------------
            case S_CK_B:
                if (c != ckB) {
                    resetFrame();
                } else {
                    // Finalise NAV-SAT special case (variable satellite blocks)
                    if (currentMsgType == MT_NAV_SAT)
                        ubxMessage.navSatCount = ubxMessage.navSatHdr.numSvs;

                    const uint8_t ret = currentMsgType;
                    resetFrame();
                    return ret;
                }
                break;
        }
    }

    return MT_NONE;
}
