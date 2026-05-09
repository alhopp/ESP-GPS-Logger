#include "Core/Globals.h"
#include "Core/build_config.h"
#include "Core/log.h"
#include "GPS/Ublox/ublox_driver.h"

// u-blox receiver configuration and time sync.
// This file owns command sequencing and GPS-derived system time updates; the
// streaming parser stays in ublox_parser.cpp.

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

    #if !STATS_ONLY_SERIAL
    Serial.println("GPS Local Time set");
    #endif
    return true;
}


// ============================================================================
// Init_ubloxM10
//
// Fully initialise a u-blox M10 GNSS receiver into a deterministic runtime
// configuration suitable for high-rate UBX-only operation.
//
// Guarantees on return:
//   • UBX output enabled and NMEA disabled
//   • GPS + GLONASS + GALILEO + BEIDOU(B1C) enabled
//   • Motion model set to SEA
//   • Navigation rate fixed at 5 Hz
//   • NAV-PVT and NAV-DOP enabled
//   • Diagnostic metadata polled (MON-VER, MON-GNSS, UID)
//   • ESP32 UART restarted and synchronised at 38400 baud
//
// Notes:
//   • Commands are deliberately sequenced with delays to match M10 behaviour
//   • This function assumes UbloxSerial pins are already configured
// ============================================================================
void Init_ubloxM10(void)
{
    constexpr uint16_t WAIT_MS = 200;

    LOG_GPS("Init", "===== u-blox M10 INIT START =====");

    // ---------------------------------------------------------------------
    // 1. Disable NMEA output (binary-only operation)
    // ---------------------------------------------------------------------
    LOG_GPS("CFG", "Disable NMEA");
    sendUbx(ubx::cfg::nmea_off);
    delay(WAIT_MS);

    LOG_GPS("CFG", "UBX protocol on");
    sendUbx(ubx::cfg::ubx_only);
    delay(WAIT_MS);

    // ---------------------------------------------------------------------
    // 2. Enable all supported GNSS constellations
    // ---------------------------------------------------------------------
    LOG_GPS("CFG", "GNSS: GPS + GAL + GLO + BDS(B1C)");
    sendUbx(ubx::cfg::m10_3gnss);
    delay(WAIT_MS);

    // ---------------------------------------------------------------------
    // 3. Set dynamic motion model
    // ---------------------------------------------------------------------
    LOG_GPS("CFG", "Motion model: SEA");
    sendUbx(ubx::cfg::sea_model);
    delay(WAIT_MS);

    // ---------------------------------------------------------------------
    // 4. Fix navigation output rate (5 Hz)
    // ---------------------------------------------------------------------
    LOG_GPS("CFG", "Nav rate: 5 Hz");
    sendUbx(ubx::rate::rate_5hz);
    delay(WAIT_MS);

    // ---------------------------------------------------------------------
    // 5. Enable required navigation messages
    // ---------------------------------------------------------------------
    LOG_GPS("MSG", "Enable NAV-PVT");
    sendUbx(ubx::msg::nav_pvt);
    delay(WAIT_MS);

    LOG_GPS("MSG", "Enable NAV-DOP");
    sendUbx(ubx::msg::nav_dop);
    delay(WAIT_MS);

    // ---------------------------------------------------------------------
    // 6. Poll diagnostic / identity information
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
    // 7. Switch GPS UART baud rate to 38400
    // ---------------------------------------------------------------------
    LOG_GPS("CFG", "Switch GPS baud → 38400");
    sendUbx(ubx::rate::baud_38400);
    delay(WAIT_MS);

    // ---------------------------------------------------------------------
    // 8. Restart ESP32 UART to match GPS baud
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

    LOG_GPS("Init", "===== u-blox M10 INIT COMPLETE =====");
}
