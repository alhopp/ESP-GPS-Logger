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


/*
void ubloxSerialInit(int delay_ms){
 for(int i=0;i<delay_ms;i++){
    int msgType = processGPS();
     if ( msgType == MT_NAV_ACK){
          Serial.print(" ACK ");
          Serial.print (ubxMessage.navAck.msg_cls);
          Serial.println (ubxMessage.navAck.msg_id);
          if(check_M10_nav_rate) High_nav_rate_ACK = true;
          }
     if ( msgType == MT_NAV_NACK){
          Serial.print(" NACK ");
          Serial.print (ubxMessage.navNack.msg_cls);
          Serial.println (ubxMessage.navNack.msg_id);
          if(check_M10_nav_rate) Nav_rate_NACK = true;
          }          
     if ( msgType == MT_NAV_ID){
          Serial.print("ID= :");
          Serial.print (ubxMessage.ubxId.ubx_id_1);
          Serial.print (ubxMessage.ubxId.ubx_id_2);
          Serial.print (ubxMessage.ubxId.ubx_id_3);
          Serial.print (ubxMessage.ubxId.ubx_id_4);
          Serial.println(ubxMessage.ubxId.ubx_id_5);
          }
     if ( msgType == MT_MON_GNSS){
          Serial.print("GNSS= :");
          Serial.print (ubxMessage.monGNSS.supported_Gnss);
          Serial.print (ubxMessage.monGNSS.default_Gnss);
          Serial.println (ubxMessage.monGNSS.enabled_Gnss);
          }
      if ( msgType == MT_MON_VER){
          Serial.print("SW Ublox=");
          Serial.println(ubxMessage.monVER.swVersion);
          Serial.print ("HW Ublox=");
          Serial.println (ubxMessage.monVER.hwVersion);
          }    
     delay(2);   
     } 
}
*/


void Init_ubloxM10(void)
{
    constexpr int WAIT_MS = 250;

    LOG_GPS("Init", "u-blox M10");

    // -------------------------------------------------------------------------
    // Transport: UBX only
    // -------------------------------------------------------------------------
    LOG_GPS("Config", "Disable NMEA");
    sendUbx(ubx::cfg::nmea_off);
    delay(WAIT_MS);

    LOG_GPS("Config", "Enable UBX output");
    sendUbx(ubx::cfg::ubx_only);
    delay(WAIT_MS);

    // -------------------------------------------------------------------------
    // GNSS constellation (atomic, M10-correct)
    // -------------------------------------------------------------------------
    LOG_GPS("Config", "GNSS: GPS + GAL + BDS(B1C) + GLO");
    sendUbx(ubx::cfg::all_4gnss);
    delay(WAIT_MS);

    // -------------------------------------------------------------------------
    // Motion model
    // -------------------------------------------------------------------------
    LOG_GPS("Config", "Motion model: SEA");
    sendUbx(ubx::cfg::sea_model);
    delay(WAIT_MS);

    // -------------------------------------------------------------------------
    // High navigation rate (optional)
    // -------------------------------------------------------------------------
    if (config.M10_high_nav == SET_M10_HIGH_NAV) {
        LOG_GPS("Init", "Enable M10 high navigation rate");
        Set_M10_high_nav_rate();   // may reboot receiver internally
        delay(WAIT_MS);
    }

    // -------------------------------------------------------------------------
    // Enable required messages
    // -------------------------------------------------------------------------
    LOG_GPS("Msg", "Enable NAV-PVT");
    sendUbx(ubx::msg::nav_pvt);
    delay(WAIT_MS);

    LOG_GPS("Msg", "Enable NAV-DOP");
    sendUbx(ubx::msg::nav_dop);
    delay(WAIT_MS);

    if (config.logUBX && config.logUBX_nav_sat) {
        Serial.println("Enable NAV-SAT");
        sendUbx(ubx::msg::nav_sat);
        delay(WAIT_MS);
    }

    // -------------------------------------------------------------------------
    // Diagnostics (still at current baud, usually 9600)
    // -------------------------------------------------------------------------
    LOG_GPS("Diag", "Query MON-VER");
    sendUbx(ubx::poll::mon_ver);
    delay(WAIT_MS);

    LOG_GPS("Diag", "Query MON-GNSS");
    sendUbx(ubx::poll::mon_gnss);
    delay(WAIT_MS);

    LOG_GPS("Diag", "Query UID");
    sendUbx(ubx::poll::uid);
    delay(WAIT_MS);

    // -------------------------------------------------------------------------
    // Enable UBX output on UART1 (MUST be before baud switch)
    // -------------------------------------------------------------------------
    LOG_GPS("Config", "Enable UART1 UBX output");
    sendUbx(ubx::cfg::uart1_ubx_out);
    delay(WAIT_MS);

    // -------------------------------------------------------------------------
    // Switch receiver baud to 38400
    // -------------------------------------------------------------------------
    LOG_GPS("Config", "Switch baud → 38400");
    sendUbx(ubx::rate::baud_38400);
    delay(WAIT_MS);

    // -------------------------------------------------------------------------
    // Restart ESP32 UART at 38400
    // -------------------------------------------------------------------------
    UbloxSerial.flush();
    UbloxSerial.end();
    delay(20);

    UbloxSerial.begin(
      38400,
      SERIAL_8N1,
      GPS_UART_RX_PIN,
      GPS_UART_TX_PIN
    );
    delay(50);

    // -------------------------------------------------------------------------
    // (Optional but rock-solid) Re-assert UART1 UBX at new baud
    // -------------------------------------------------------------------------
    LOG_GPS("Config", "Re-assert UART1 UBX @38400");
    sendUbx(ubx::cfg::uart1_ubx_out);  
    delay(WAIT_MS);

    LOG_GPS("Init", "complete");
}



// -----------------------------------------------------------------------------
// Set_rate_ubloxM10
//
// Configure navigation output rate for u-blox M10 using prebuilt UBX commands.
//
// Supported rates (Hz):
//   1, 2, 4, 5, 8, 10, 15, 20
//
// Each rate corresponds to one 18-byte UBX-CFG-RATE command
// stored sequentially in ubx::rate::table[].
//
// NOTE:
// - M10 does NOT accept arbitrary rates via a single parameter
// - Each supported rate must have its own UBX command
// -----------------------------------------------------------------------------
void Set_rate_ubloxM10(int rate_hz)
{
    constexpr int CMD_SIZE = 18;

    // Map requested rate → command index
    int index = -1;

    switch (rate_hz) {
        case 1:  index = 0; break;
        case 2:  index = 1; break;
        case 4:  index = 2; break;
        case 5:  index = 3; break;
        case 8:  index = 4; break;
        case 10: index = 5; break;
        case 15: index = 6; break;
        case 20: index = 7; break;
        default:
            Serial.printf("Unsupported M10 rate %d Hz → fallback to 1 Hz\n", rate_hz);
            rate_hz = 1;
            index   = 0;
            config.sample_rate = 1;
            break;
    }

    Serial.printf("Set u-blox M10 nav rate: %d Hz\n", rate_hz);

    const int offset = index * CMD_SIZE;

    for (int i = 0; i < CMD_SIZE; i++) {
        UbloxSerial.write(pgm_read_byte(ubx::rate::table + offset + i));
    }

    delay(500);
}


// The last two bytes of the message is a checksum value, used to confirm that the received payload is valid.
// The procedure used to calculate this is given as pseudo-code in the uBlox manual.
void calcChecksum(unsigned char* CK,int msgType, int msgSize) {
  memset(CK, 0, 2);
  for (int i = 0; i < msgSize; i++) {
    if(msgType==MT_NAV_PVT) {CK[0] += ((unsigned char*)(&ubxMessage.navPvt))[i];}
    else if(msgType==MT_NAV_DOP) {CK[0] += ((unsigned char*)(&ubxMessage.navDOP))[i];} 
    else if(msgType==MT_MON_GNSS){CK[0] += ((unsigned char*)(&ubxMessage.monGNSS))[i];} 
    else if(msgType==MT_MON_VER){CK[0] += ((unsigned char*)(&ubxMessage.monVER))[i];} 
    else if(msgType==MT_NAV_ACK){CK[0] += ((unsigned char*)(&ubxMessage.navAck))[i];} 
    else if(msgType==MT_NAV_NACK){CK[0] += ((unsigned char*)(&ubxMessage.navNack))[i];} 
    else if(msgType==MT_NAV_SAT){CK[0] += ((unsigned char*)(&ubxMessage.navSat))[i];} 
    else if(msgType==MT_NAV_ID){CK[0] += ((unsigned char*)(&ubxMessage.ubxId))[i];} 
    else {CK[0] += ((unsigned char*)(&ubxMessage))[i];}
    CK[1] += CK[0];
  }
}
// Compares the first two bytes of the ubxMessage struct with a specific message header.
// Returns true if the two bytes match (0xB5 0x62).
boolean compareMsgHeader(const unsigned char* msgHeader) {
  unsigned char* ptr = (unsigned char*)(&ubxMessage.navDummy);
  return ptr[0] == msgHeader[0] && ptr[1] == msgHeader[1];
}
// Reads in bytes from the GPS module and checks to see if a valid message has been constructed.
// Returns the type of the message found if successful, or MT_NONE if no message was found.
// After a successful return the contents of the ubxMessage union will be valid, for the 
// message type that was found. As now every message has its own struct, further calls to this function can invalidate the
// message content if the message was the same, so you must use the obtained values before calling this function again.

int processGPS() {
  static int fpos = 0;
  static uint8_t checksum[2];
  static uint8_t currentMsgType = MT_NONE;
  static int payloadSize = sizeof(ubxMessage.navDummy);

  while (UbloxSerial.available()) {
    uint8_t c = UbloxSerial.read();

    // ------------------------------------------------------------------
    // Sync on UBX header
    // ------------------------------------------------------------------
    if (fpos < 2) {
      if (c == UBX_HEADER[fpos]) fpos++;
      else fpos = 0;
      continue;
    }

    // ------------------------------------------------------------------
    // Read header + payload (excluding sync bytes)
    // ------------------------------------------------------------------
    if ((fpos - 2) < payloadSize && fpos < 4) {
      ((uint8_t*)&ubxMessage.navDummy)[fpos - 2] = c;
    }

    // ------------------------------------------------------------------
    // Identify message type (after cls + id)
    // ------------------------------------------------------------------
    if (fpos == 3) {
      if (compareMsgHeader(NAV_PVT_HEADER)) {
        currentMsgType = MT_NAV_PVT;
        payloadSize = sizeof(NAV_PVT);
        ubxMessage.navPvt.cls = ubxMessage.navDummy.cls;
        ubxMessage.navPvt.id  = ubxMessage.navDummy.id;
      }
      else if (compareMsgHeader(MON_GNSS_HEADER)) {
        currentMsgType = MT_MON_GNSS;
        payloadSize = sizeof(MON_GNSS);
        ubxMessage.monGNSS.cls = ubxMessage.navDummy.cls;
        ubxMessage.monGNSS.id  = ubxMessage.navDummy.id;
      }
      else if (compareMsgHeader(NAV_DOP_HEADER)) {
        currentMsgType = MT_NAV_DOP;
        payloadSize = sizeof(NAV_DOP);
        ubxMessage.navDOP.cls = ubxMessage.navDummy.cls;
        ubxMessage.navDOP.id  = ubxMessage.navDummy.id;
      }
      else if (compareMsgHeader(MON_VER_HEADER)) {
        currentMsgType = MT_MON_VER;
        payloadSize = sizeof(MON_VER);
        ubxMessage.monVER.cls = ubxMessage.navDummy.cls;
        ubxMessage.monVER.id  = ubxMessage.navDummy.id;
      }
      else if (compareMsgHeader(NAV_ACK_HEADER)) {
        currentMsgType = MT_NAV_ACK;
        payloadSize = sizeof(NAV_ACK);
        ubxMessage.navAck.cls = ubxMessage.navDummy.cls;
        ubxMessage.navAck.id  = ubxMessage.navDummy.id;
      }
      else if (compareMsgHeader(NAV_NACK_HEADER)) {
        currentMsgType = MT_NAV_NACK;
        payloadSize = sizeof(NAV_NACK);
        ubxMessage.navNack.cls = ubxMessage.navDummy.cls;
        ubxMessage.navNack.id  = ubxMessage.navDummy.id;
      }
      else if (compareMsgHeader(NAV_SAT_HEADER)) {
        currentMsgType = MT_NAV_SAT;
        ubxMessage.navSatHdr.cls = ubxMessage.navDummy.cls;
        ubxMessage.navSatHdr.id  = ubxMessage.navDummy.id;
      }
      else if (compareMsgHeader(NAV_ID_HEADER)) {
        currentMsgType = MT_NAV_ID;
        ubxMessage.ubxId.cls = ubxMessage.navDummy.cls;
        ubxMessage.ubxId.id  = ubxMessage.navDummy.id;
      }
      else {
        currentMsgType = MT_NONE;
        fpos = 0;
        continue;
      }
    }

    // ------------------------------------------------------------------
    // Store payload bytes
    // ------------------------------------------------------------------
    if ((fpos - 2) < payloadSize && fpos >= 4) {
      switch (currentMsgType) {
        case MT_NAV_PVT:  ((uint8_t*)&ubxMessage.navPvt)[fpos - 2] = c; break;
        case MT_NAV_DOP:  ((uint8_t*)&ubxMessage.navDOP)[fpos - 2] = c; break;
        case MT_MON_GNSS: ((uint8_t*)&ubxMessage.monGNSS)[fpos - 2] = c; break;
        case MT_MON_VER:  ((uint8_t*)&ubxMessage.monVER)[fpos - 2] = c; break;
        case MT_NAV_ACK:  ((uint8_t*)&ubxMessage.navAck)[fpos - 2] = c; break;
        case MT_NAV_NACK: ((uint8_t*)&ubxMessage.navNack)[fpos - 2] = c; break;
        case MT_NAV_ID:   ((uint8_t*)&ubxMessage.ubxId)[fpos - 2] = c; break;

        case MT_NAV_SAT: {
          // Header first, then satellite blocks
          if ((fpos - 2) < sizeof(NAV_SAT_HDR)) {
            ((uint8_t*)&ubxMessage.navSatHdr)[fpos - 2] = c;
          } else {
            uint16_t satOfs = (fpos - 2) - sizeof(NAV_SAT_HDR);
            if (satOfs < sizeof(ubxMessage.navSat)) {
              ((uint8_t*)ubxMessage.navSat)[satOfs] = c;
            }
          }
        } break;
      }
    }

    // ------------------------------------------------------------------
    // Adjust payload size once LEN is known
    // ------------------------------------------------------------------
    if (fpos == 6) {
      if (currentMsgType == MT_NAV_PVT) ubxMessage.navPvt.len = payloadSize - 6;
      if (currentMsgType == MT_NAV_DOP) ubxMessage.navDOP.len = payloadSize - 6;

      if (currentMsgType == MT_NAV_ID) {
        payloadSize = ubxMessage.ubxId.len + 6;
      }

      if (currentMsgType == MT_MON_VER) {
        if (ubxMessage.monVER.len + 6 < sizeof(ubxMessage.monVER))
          payloadSize = ubxMessage.monVER.len + 6;
        else
          fpos = 0;
      }

      if (currentMsgType == MT_NAV_SAT) {
        uint16_t fullLen = ubxMessage.navSatHdr.len + 6;
        if (fullLen <= sizeof(NAV_SAT_HDR) + sizeof(ubxMessage.navSat))
          payloadSize = fullLen;
        else
          fpos = 0;
      }
    }

    fpos++;

    // ------------------------------------------------------------------
    // Checksum handling
    // ------------------------------------------------------------------
    if (fpos == payloadSize) {
      calcChecksum(checksum, currentMsgType, payloadSize - 2);
    }
    else if (fpos == payloadSize + 1) {
      if (c != checksum[0]) {
        fpos = 0;
      }
    }
    else if (fpos == payloadSize + 2) {
      fpos = 0;
      if (c == checksum[1]) {
        if (currentMsgType == MT_NAV_SAT) {
          ubxMessage.navSatCount = ubxMessage.navSatHdr.numSvs;
        }
        return currentMsgType;
      }
    }
    else if (fpos > payloadSize + 2) {
      fpos = 0;
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



int Check_M10_nav_rate(void){
  int result=NO_M10_GPS;
  check_M10_nav_rate=true;
  Serial.println("Get M10 NAV Rate ");
    for(int i = 0; i < sizeof(ubx::highnav::get_nav_rate); i++) {                        
    UbloxSerial.write( pgm_read_byte(ubx::highnav::get_nav_rate+i) );
    }
  Nav_rate_NACK = false;
  High_nav_rate_ACK = false;      
  delay(500);
  check_M10_nav_rate = false;
  if(Nav_rate_NACK){ 
    Serial.println("M10 Default Nav Rate");
    result=M10_DEFAULT_NAV;
    if (EEPROM.readByte(1) != M10_DEFAULT_NAV) {
     EEPROM.writeByte(1, M10_DEFAULT_NAV);
     EEPROM.commit();
    }
  }
  if(High_nav_rate_ACK){
    Serial.println("M10 High Nav Rate set");
    result=M10_HIGH_NAV_RATE;

    if (EEPROM.readByte(1) != M10_HIGH_NAV_RATE) {
     EEPROM.writeByte(1, M10_HIGH_NAV_RATE);
     EEPROM.commit();
    }

    } 
    return result;       
  }
int Set_M10_high_nav_rate(void){
  Serial.println("Set ublox UBX_M10 High Nav Rate");
  for(int i = 0; i < sizeof(ubx::highnav::set_high_nav_rate); i++) {                        
     UbloxSerial.write( pgm_read_byte(ubx::highnav::set_high_nav_rate+i) );
     }
  delay(500); 
  config.ublox_type= UBLOX_TYPE_UNKNOWN;
  config.M10_high_nav= M10_HIGH_NAV_RATE;
  EEPROM.writeByte(0,UBLOX_TYPE_UNKNOWN);
  EEPROM.writeByte(1,M10_HIGH_NAV_RATE); EEPROM.commit();//always set EEPROM to default nav rate.....
  return config.M10_high_nav;
}  
