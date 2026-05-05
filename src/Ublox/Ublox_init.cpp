#include "Ublox/Ublox.h"
#include "Core/Definitions.h"
#include "Core/rtc_state.h"
#include "Core/Globals.h"
#include "Config/config_types.h"

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
    t.tm_sec  = pvt.sec;
    t.tm_min  = pvt.min;
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
    sendUbx(ubx::cfg::m10_3gnss);
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
