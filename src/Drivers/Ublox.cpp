#include "Ublox.h"
#include "Definitions.h"
#include <EEPROM.h>
#include "config_manager.h"
#include "rtc_state.h"

int Time_Set_OK;
bool Nav_rate_NACK = false;
bool High_nav_rate_ACK = false;
bool check_M10_nav_rate = false;

UBXMessage ubxMessage = {};

struct tm tmstruct ;
struct tm my_time;      // time elements structure
time_t unix_timestamp;  // a timestamp

bool Set_GPS_Time(float time_offset)
{
    // Reject clearly invalid GPS time
    if (ubxMessage.navPvt.year < 2023) {
        return false;
    }

    struct tm my_time = {};
    my_time.tm_sec  = ubxMessage.navPvt.second;
    my_time.tm_min  = ubxMessage.navPvt.minute;
    my_time.tm_hour = ubxMessage.navPvt.hour;
    my_time.tm_mday = ubxMessage.navPvt.day;
    my_time.tm_mon  = ubxMessage.navPvt.month - 1;      // 0–11
    my_time.tm_year = ubxMessage.navPvt.year - 1900;    // years since 1900

    // Apply timezone offset (hours)
    my_time.tm_hour += (int)time_offset;

    // Force UTC for conversion
    setenv("TZ", "UTC0", 1);
    tzset();

    time_t unix_time = mktime(&my_time);
    if (unix_time < 1672531200) { // sanity: < 2023-01-01
        return false;
    }

    struct timeval tv;
    tv.tv_sec  = unix_time;
    tv.tv_usec = 0;
    settimeofday(&tv, nullptr);

    // Restore local timezone
    setenv("TZ", TimeZone, 1);
    tzset();

    // Verify result
    struct tm tmstruct;
    if (!getLocalTime(&tmstruct)) {
        return false;
    }

    if ((tmstruct.tm_year + 1900) < 2023) {
        return false;
    }

    Serial.println("GPS Local Time set");
    return true;
}


const char* gpsChip(bool longname)
{
    return longname ? "u-blox M10 (UBX)" : "M10";
}

HardwareSerial UbloxSerial(2);

void ubloxSerialInit(int delay_ms)
{
  UbloxSerial.begin(38400, SERIAL_8N1, GPS_UART_RX_PIN, GPS_UART_TX_PIN);
  delay(delay_ms);
}

// CFG-PRT: UART1, UBX IN + OUT, NMEA OFF
// u-blox M10 compatible
namespace ubx::cfg {
  const uint8_t uart1_ubx_in_out[] PROGMEM = {
    0xB5,0x62,0x06,0x00,0x14,0x00,
    0x01,0x00,          // portID = UART1
    0x00,0x00,          // txReady
    0xD0,0x08,0x00,0x00,// mode = 8N1
    0x00,0x96,0x00,0x00,// baud = 38400 (overwritten if earlier)
    0x01,0x00,          // inProtoMask  = UBX
    0x01,0x00,          // outProtoMask = UBX
    0x00,0x00,          // flags
    0x00,0x00,          // reserved
    0xA2,0xB5           // CK_A CK_B
  };
}

void Init_ubloxM10(void)
{
    constexpr int WAIT = 200;

    LOG_GPS("Init", "===== u-blox M10 INIT START =====");

    // ---------------------------------------------------------------------
    // Enable UBX IN + OUT on UART1 (CRITICAL)
    // ---------------------------------------------------------------------
    LOG_GPS("CFG", "UART1 UBX IN+OUT");
    sendUbx(ubx::cfg::uart1_ubx_in_out);
    delay(WAIT);

    // ---------------------------------------------------------------------
    // Disable NMEA
    // ---------------------------------------------------------------------
    LOG_GPS("CFG", "Disable NMEA");
    sendUbx(ubx::cfg::nmea_off);
    delay(WAIT);

    // ---------------------------------------------------------------------
    // GNSS constellation
    // ---------------------------------------------------------------------
    LOG_GPS("CFG", "GNSS: GPS + GAL + GLO + BDS(B1C)");
    sendUbx(ubx::cfg::all_4gnss);
    delay(WAIT);

    // ---------------------------------------------------------------------
    // Motion model
    // ---------------------------------------------------------------------
    LOG_GPS("CFG", "Motion model: SEA");
    sendUbx(ubx::cfg::sea_model);
    delay(WAIT);

    // ---------------------------------------------------------------------
    // Navigation rate (fixed 5 Hz)
    // ---------------------------------------------------------------------
    LOG_GPS("CFG", "Nav rate: fixed 5 Hz");
    sendUbx(ubx::rate::rate_5hz);
    delay(WAIT);

    // ---------------------------------------------------------------------
    // Enable NAV messages
    // ---------------------------------------------------------------------
    LOG_GPS("MSG", "Enable NAV-PVT");
    sendUbx(ubx::msg::nav_pvt);
    delay(WAIT);

    LOG_GPS("MSG", "Enable NAV-DOP");
    sendUbx(ubx::msg::nav_dop);
    delay(WAIT);

    if (config.logUBX && config.logUBX_nav_sat) {
        LOG_GPS("MSG", "Enable NAV-SAT");
        sendUbx(ubx::msg::nav_sat);
        delay(WAIT);
    }

    // ---------------------------------------------------------------------
    // Diagnostics polls (VALID ones)
    // ---------------------------------------------------------------------
    LOG_GPS("POLL", "MON-VER");
    sendUbx(ubx::poll::mon_ver);
    delay(WAIT);

    LOG_GPS("POLL", "MON-GNSS");
    sendUbx(ubx::poll::mon_gnss);
    delay(WAIT);

    LOG_GPS("POLL", "UID");
    sendUbx(ubx::poll::uid);
    delay(WAIT);

    // ---------------------------------------------------------------------
    // Switch GPS baud → 38400
    // ---------------------------------------------------------------------
    LOG_GPS("CFG", "Switch GPS baud → 38400");
    sendUbx(ubx::rate::baud_38400);
    delay(WAIT);

    // ---------------------------------------------------------------------
    // Restart ESP32 UART
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
    // Re-assert UBX IN+OUT @ new baud
    // ---------------------------------------------------------------------
    LOG_GPS("CFG", "Re-assert UART1 UBX IN+OUT @38400");
    sendUbx(ubx::cfg::uart1_ubx_in_out);
    delay(WAIT);

    LOG_GPS("Init", "===== u-blox M10 INIT COMPLETE =====");
}




// Reads in bytes from the GPS module and checks to see if a valid message has been constructed.
// Returns the type of the message found if successful, or MT_NONE if no message was found.
// After a successful return the contents of the ubxMessage union will be valid, for the 
// message type that was found. As now every message has its own struct, further calls to this function can invalidate the
// message content if the message was the same, so you must use the obtained values before calling this function again.

int processGPS()
{
  // UBX framing:
  // B5 62 | CLS ID | LEN_L LEN_H | PAYLOAD[len] | CK_A CK_B
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

  static State   st        = S_SYNC1;
  static uint8_t cls       = 0;
  static uint8_t id        = 0;
  static uint16_t len      = 0;
  static uint16_t payPos   = 0;

  static uint8_t ckA = 0;
  static uint8_t ckB = 0;

  // Small payload buffer for “unknown” messages (or for safety)
  // NAV-PVT payload = 92, NAV-DOP payload = 18, NAV-SAT can be large.
  // We'll stream-copy directly into structs where possible.
  // This buffer is only used if we don't recognise the message.
  static uint8_t scratch[256];

  // Optional: RAW throughput monitor (non-destructive-ish)
  static uint32_t lastRawLog = 0;
  static uint32_t rawCount   = 0;
  if (UbloxSerial.available()) {
    rawCount++;
    if (millis() - lastRawLog > 1000) {
      lastRawLog = millis();
      uint8_t b = UbloxSerial.peek();
      LOG_GPS("RAW", "bytes/sec=%lu first=0x%02X", (unsigned long)rawCount, b);
      rawCount = 0;
    }
  }

  auto resetFrame = [&]() {
    st = S_SYNC1;
    cls = id = 0;
    len = 0;
    payPos = 0;
    ckA = ckB = 0;
  };

  auto beginChecksum = [&]() {
    ckA = 0;
    ckB = 0;
  };

  auto addChecksum = [&](uint8_t b) {
    ckA = (uint8_t)(ckA + b);
    ckB = (uint8_t)(ckB + ckA);
  };

  // Decide where payload bytes should go
  auto startMessage = [&]() -> uint8_t {
    // Identify message type from cls/id
    if (cls == 0x01 && id == 0x07) { LOG_GPS("HDR", "NAV-PVT"); return MT_NAV_PVT; }
    if (cls == 0x01 && id == 0x04) { LOG_GPS("HDR", "NAV-DOP"); return MT_NAV_DOP; }
    if (cls == 0x0A && id == 0x28) { LOG_GPS("HDR", "MON-GNSS"); return MT_MON_GNSS; }
    if (cls == 0x0A && id == 0x04) { LOG_GPS("HDR", "MON-VER"); return MT_MON_VER; }
    if (cls == 0x05 && id == 0x01) { LOG_GPS("HDR", "ACK"); return MT_NAV_ACK; }
    if (cls == 0x05 && id == 0x00) { LOG_GPS("HDR", "NACK"); return MT_NAV_NACK; }
    if (cls == 0x27 && id == 0x03) { LOG_GPS("HDR", "NAV-ID"); return MT_NAV_ID; }
    if (cls == 0x01 && id == 0x35) { LOG_GPS("HDR", "NAV-SAT"); return MT_NAV_SAT; }
    // Unknown
    // LOG_GPS("HDR", "UNK %02X %02X", cls, id);
    return MT_NONE;
  };

  static uint8_t currentMsgType = MT_NONE;

  while (UbloxSerial.available()) {
    uint8_t c = UbloxSerial.read();

    switch (st) {

      case S_SYNC1:
        if (c == 0xB5) st = S_SYNC2;
        break;

      case S_SYNC2:
        if (c == 0x62) {
          st = S_CLS;
        } else {
          // allow re-sync if we saw another 0xB5
          st = (c == 0xB5) ? S_SYNC2 : S_SYNC1;
        }
        break;

      case S_CLS:
        cls = c;
        beginChecksum();
        addChecksum(c);
        st = S_ID;
        break;

      case S_ID:
        id = c;
        addChecksum(c);
        st = S_LEN1;
        break;

      case S_LEN1:
        len = c;
        addChecksum(c);
        st = S_LEN2;
        break;

      case S_LEN2:
        len |= (uint16_t)c << 8;
        addChecksum(c);

        payPos = 0;
        currentMsgType = startMessage();

        // Basic sanity: UBX payload length can be bigger than our structures.
        // We'll clamp/copy safely.
        st = (len == 0) ? S_CK_A : S_PAYLOAD;
        break;

      case S_PAYLOAD: {
        // Stream-copy payload byte c to the right destination
        // based on currentMsgType and payload position payPos.

        // 1) NAV-PVT payload-only struct (92 bytes)
        if (currentMsgType == MT_NAV_PVT) {
          if (payPos < sizeof(NAV_PVT)) {
            ((uint8_t*)&ubxMessage.navPvt)[payPos] = c;
          }
        }
        // 2) NAV-DOP includes cls/id/len in struct, payload starts at +4
        else if (currentMsgType == MT_NAV_DOP) {
          if (payPos == 0) { ubxMessage.navDOP.cls = cls; ubxMessage.navDOP.id = id; ubxMessage.navDOP.len = len; }
          if (payPos < (sizeof(NAV_DOP) - 4)) {
            ((uint8_t*)&ubxMessage.navDOP)[4 + payPos] = c;
          }
        }
        // 3) MON-GNSS includes cls/id/len
        else if (currentMsgType == MT_MON_GNSS) {
          if (payPos == 0) { ubxMessage.monGNSS.cls = cls; ubxMessage.monGNSS.id = id; ubxMessage.monGNSS.len = len; }
          if (payPos < (sizeof(MON_GNSS) - 4)) {
            ((uint8_t*)&ubxMessage.monGNSS)[4 + payPos] = c;
          }
        }
        // 4) MON-VER includes cls/id/len, variable payload -> clamp to struct capacity
        else if (currentMsgType == MT_MON_VER) {
          if (payPos == 0) { ubxMessage.monVER.cls = cls; ubxMessage.monVER.id = id; ubxMessage.monVER.len = len; }
          const uint16_t cap = (uint16_t)(sizeof(MON_VER) - 4);
          if (payPos < cap) {
            ((uint8_t*)&ubxMessage.monVER)[4 + payPos] = c;
          }
        }
        // 5) ACK/NACK include cls/id/len
        else if (currentMsgType == MT_NAV_ACK) {
          if (payPos == 0) { ubxMessage.navAck.cls = cls; ubxMessage.navAck.id = id; ubxMessage.navAck.len = len; }
          const uint16_t cap = (uint16_t)(sizeof(NAV_ACK) - 4);
          if (payPos < cap) ((uint8_t*)&ubxMessage.navAck)[4 + payPos] = c;
        }
        else if (currentMsgType == MT_NAV_NACK) {
          if (payPos == 0) { ubxMessage.navNack.cls = cls; ubxMessage.navNack.id = id; ubxMessage.navNack.len = len; }
          const uint16_t cap = (uint16_t)(sizeof(NAV_NACK) - 4);
          if (payPos < cap) ((uint8_t*)&ubxMessage.navNack)[4 + payPos] = c;
        }
        // 6) NAV-ID includes cls/id/len (your struct includes header bytes)
        else if (currentMsgType == MT_NAV_ID) {
          if (payPos == 0) { ubxMessage.ubxId.cls = cls; ubxMessage.ubxId.id = id; ubxMessage.ubxId.len = len; }
          const uint16_t cap = (uint16_t)(sizeof(NAV_ID) - 4);
          if (payPos < cap) ((uint8_t*)&ubxMessage.ubxId)[4 + payPos] = c;
        }
        // 7) NAV-SAT: payload begins with iTOW/version/numSvs/r1/r2 then blocks
        else if (currentMsgType == MT_NAV_SAT) {
          if (payPos == 0) { ubxMessage.navSatHdr.cls = cls; ubxMessage.navSatHdr.id = id; ubxMessage.navSatHdr.len = len; }
          // Copy payload into the navSatHdr fields after the first 4 bytes
          const uint16_t hdrPayloadCap = (uint16_t)(sizeof(NAV_SAT_HDR) - 4);
          if (payPos < hdrPayloadCap) {
            ((uint8_t*)&ubxMessage.navSatHdr)[4 + payPos] = c;
          } else {
            // Satellite blocks start after NAV_SAT_HDR payload (which is 8 bytes: iTOW(4)+ver+numSvs+r1+r2)
            const uint16_t satOfs = (uint16_t)(payPos - hdrPayloadCap);
            const uint16_t satCapBytes = (uint16_t)(sizeof(ubxMessage.navSat));
            if (satOfs < satCapBytes) {
              ((uint8_t*)ubxMessage.navSat)[satOfs] = c;   // byte-wise into array memory
            }
          }
        }
        // Unknown: store up to scratch size
        else {
          if (payPos < sizeof(scratch)) scratch[payPos] = c;
        }

        addChecksum(c);
        payPos++;

        if (payPos >= len) {
          st = S_CK_A;
        }
      } break;

      case S_CK_A:
        // Compare received CK_A
        if (c != ckA) {
          LOG_GPS("CK", "FAIL A cls=%02X id=%02X len=%u got=%02X exp=%02X",
                  cls, id, (unsigned)len, c, ckA);
          resetFrame();
        } else {
          st = S_CK_B;
        }
        break;

      case S_CK_B:
        if (c != ckB) {
          LOG_GPS("CK", "FAIL B cls=%02X id=%02X len=%u got=%02X exp=%02X",
                  cls, id, (unsigned)len, c, ckB);
          resetFrame();
        } else {
          // Valid full frame
          if (currentMsgType == MT_NAV_SAT) {
            ubxMessage.navSatCount = ubxMessage.navSatHdr.numSvs;
          }
          // Optional “good frame” log:
          // LOG_GPS("OK", "cls=%02X id=%02X len=%u type=%u", cls, id, (unsigned)len, currentMsgType);

          uint8_t ret = currentMsgType;
          resetFrame();
          return ret;
        }
        break;
    }
  }

  return MT_NONE;
}






bool Check_ublox_M10()
{
    // Assume UbloxSerial is already configured to the expected baud
    Serial.println("Checking u-blox M10...");

    for (int i = 0; i < sizeof(ubx::poll::mon_ver); i++) {
        UbloxSerial.write(pgm_read_byte(ubx::poll::mon_ver + i));
    }

    delay(500);

    // M10 hardware string typically contains 'A'
    if (ubxMessage.monVER.hwVersion[3] == 'A') {
        Serial.println("u-blox M10 detected");
        return true;
    }

    Serial.println("ERROR: u-blox M10 not responding");
    return false;
}

