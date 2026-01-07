#include "Ublox.h"
#include "Definitions.h"
#include <EEPROM.h>
#include "config_manager.h"
#include "rtc_state.h"

int Time_Set_OK;
bool Nav_rate_NACK = false;
bool High_nav_rate_ACK = false;
bool check_M10_nav_rate = false;



//UBXMessage ubxMessage = {000000000000};//definition here, declaration in ublox.h !!
UBXMessage ubxMessage = {};

struct tm tmstruct ;
struct tm my_time;  // time elements structure
time_t unix_timestamp; // a timestamp


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



void Ublox_serial2(int delay_ms){
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


void Init_ublox(void)
{
    // LEGACY M8 CODE — DISABLED
    // M10-only firmware
    Serial.println("ERROR: Init_ublox() called (M8 legacy)");
    while (1) delay(1000);
}


// -----------------------------------------------------------------------------
// Helper for Init_ubloxM10
//
#define SEND_UBX(arr)                           \
    do {                                       \
        for (int i = 0; i < sizeof(arr); i++) \
            Serial2.write(pgm_read_byte(arr + i)); \
        Ublox_serial2(WAIT_MS);                \
    } while (0)


// -----------------------------------------------------------------------------
// Init_ubloxM10
//
// One-time initialization for u-blox M10 receivers.
//
// Design assumptions:
// - M10 only (no M8 / M9 support)
// - SEA motion model (fixed, no runtime switching)
// - UBX protocol only (NMEA disabled)
// - Baudrate switched to 38400 for >=5 Hz operation
//
// This function must be called ONCE at startup.
// -----------------------------------------------------------------------------
void Init_ubloxM10(void)
{
    constexpr int WAIT_MS = 250;

    Serial.println("Init u-blox M10");

    // -------------------------------------------------------------------------
    // Disable all NMEA output (UBX only)
    // -------------------------------------------------------------------------
    Serial.println("Disable NMEA");
    for (int i = 0; i < sizeof(UBLOX_M10_NMEA_OFF); i++) {
        Serial2.write(pgm_read_byte(UBLOX_M10_NMEA_OFF + i));
    }
    Ublox_serial2(WAIT_MS);

    // -------------------------------------------------------------------------
    // High navigation rate (optional)
    // -------------------------------------------------------------------------
    if (config.M10_high_nav == SET_M10_HIGH_NAV) {
        Serial.println("Enable M10 high navigation rate");
        Set_M10_high_nav_rate();   // may reboot receiver internally
        Ublox_serial2(WAIT_MS);
    }

    // -------------------------------------------------------------------------
    // Motion model: SEA (fixed)
    // -------------------------------------------------------------------------
    Serial.println("Set motion model: SEA");
    for (int i = 0; i < sizeof(UBX_M10_SEA); i++) {
        Serial2.write(pgm_read_byte(UBX_M10_SEA + i));
    }
    Ublox_serial2(WAIT_MS);

    // -------------------------------------------------------------------------
    // GNSS constellation selection
    // -------------------------------------------------------------------------
    // Default M10 = GPS + GALILEO + BEIDOU(B1)
    switch (config.gnss) {

        case 4: // GPS + GALILEO + BEIDOU(B1C)
            Serial.println("GNSS: GPS + GALILEO + BEIDOU(B1C)");
            SEND_UBX(UBLOX_M10_GLONAS_OFF);
            SEND_UBX(UBLOX_M10_BEIDOU_B1_OFF);
            SEND_UBX(UBLOX_M10_BEIDOU_B1C_ON);
            break;

        case 3: // GPS + GALILEO + GLONASS
            Serial.println("GNSS: GPS + GALILEO + GLONASS");
            SEND_UBX(UBLOX_M10_BEIDOU_OFF);
            SEND_UBX(UBLOX_M10_GLONAS_ON);
            break;

        case 2: // GPS + GLONASS
            Serial.println("GNSS: GPS + GLONASS");
            SEND_UBX(UBLOX_M10_GAL_OFF);
            SEND_UBX(UBLOX_M10_BEIDOU_OFF);
            SEND_UBX(UBLOX_M10_GLONAS_ON);
            break;

        case 1: // GPS + GALILEO
            Serial.println("GNSS: GPS + GALILEO");
            SEND_UBX(UBLOX_M10_BEIDOU_OFF);
            SEND_UBX(UBLOX_M10_GLONAS_OFF);
            break;

        default:
            Serial.println("GNSS: default (M10)");
            break;
    }

    // -------------------------------------------------------------------------
    // Enable required UBX messages (M10 ONLY)
    // -------------------------------------------------------------------------

    Serial.println("Enable UBX output");
    SEND_UBX(UBLOX_M10_UBX);

    Serial.println("Enable NAV-PVT");
    SEND_UBX(UBLOX_M10_NAV_PVT);

    Serial.println("Enable NAV-DOP");
    SEND_UBX(UBLOX_M10_NAV_DOP);

    if (config.logUBX && config.logUBX_nav_sat) {
        Serial.println("Enable NAV-SAT");
        SEND_UBX(UBLOX_M10_NAV_SAT);
    }

    // -------------------------------------------------------------------------
    // Diagnostics
    // -------------------------------------------------------------------------
    Serial.println("Query MON-VER");
    SEND_UBX(UBX_MON_VER);

    Serial.println("Query MON-GNSS");
    SEND_UBX(UBX_MON_GNSS);

    Serial.println("Query unique ID");
    SEND_UBX(UBX_ID);

    // -------------------------------------------------------------------------
    // Switch baudrate to 38400 (final step)
    // -------------------------------------------------------------------------
    Serial.println("Switch baudrate to 38400");
    SEND_UBX(UBLOX_M10_UBX_BD38400);

    Serial2.flush();
    Serial2.begin(38400, SERIAL_8N1,
                  GPS_UART_RX_PIN,
                  GPS_UART_TX_PIN);

    Ublox_serial2(WAIT_MS);

    Serial.println("u-blox M10 init complete");
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
// stored sequentially in UBLOX_M10_RATE[].
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
        Serial2.write(pgm_read_byte(UBLOX_M10_RATE + offset + i));
    }

    Ublox_serial2(500);
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
  static unsigned char checksum[2];
  static byte currentMsgType = MT_NONE;
  static int payloadSize = sizeof(ubxMessage.navDummy);
  //static uint16_t len;//10 Hz M10 issue ??
  while ( Serial2.available() ) {
    byte c = Serial2.read();    
    //Serial.write(c);
    if ( fpos < 2 ) {
      // For the first two bytes we are simply looking for a match with the UBX header bytes (0xB5,0x62)
      if ( c == UBX_HEADER[fpos] )
        fpos++;
      else
        fpos = 0; // Reset to beginning state.
    }
    else {
      // If we come here then fpos >= 2, which means we have found a match with the UBX_HEADER
      // and we are now reading in the bytes that make up the payload.
      // Place the incoming byte into the ubxMessage.navDummy struct. The position is fpos-2 because
      // the struct does not include the initial two-byte header (UBX_HEADER).
      // the struct does not include the 2 last bytes which are the checksums
      // checksums are not placed in the ubxMessage !!!
      if (((fpos-2) < payloadSize)&&(fpos<4)){((unsigned char*)(&ubxMessage.navDummy))[fpos-2] = c;} 
      if(fpos==3) {
        // We have just received the second byte of the message type header, 
        // so now we can check to see what kind of message it is.
        //We have to restore cls and id to the correct substructure
        if ( compareMsgHeader(NAV_PVT_HEADER) ) {
          currentMsgType = MT_NAV_PVT;
          payloadSize = sizeof(NAV_PVT);
          ubxMessage.navPvt.cls=ubxMessage.navDummy.cls;
          ubxMessage.navPvt.id=ubxMessage.navDummy.id;
          //Serial.print("hPVT ");
        }
        else if ( compareMsgHeader(MON_GNSS_HEADER) ) {
          currentMsgType = MT_MON_GNSS;
          payloadSize = sizeof(MON_GNSS);
          ubxMessage.monGNSS.cls=ubxMessage.navDummy.cls;
          ubxMessage.monGNSS.id=ubxMessage.navDummy.id;
          //Serial.println("MT_MON_GNSS\n");
        }
        else if ( compareMsgHeader(NAV_DOP_HEADER) ) {
          currentMsgType = MT_NAV_DOP;
          payloadSize = sizeof(NAV_DOP);
          ubxMessage.navDOP.cls=ubxMessage.navDummy.cls;
          ubxMessage.navDOP.id=ubxMessage.navDummy.id;
          //Serial.print("hDOP ");
        }
        else if ( compareMsgHeader(MON_VER_HEADER) ) {
          currentMsgType = MT_MON_VER;
          payloadSize = sizeof(MON_VER);
          ubxMessage.monVER.cls=ubxMessage.navDummy.cls;
          ubxMessage.monVER.id=ubxMessage.navDummy.id;
          //Serial.println("MT_MON_VER\n");
        }
        else if ( compareMsgHeader(NAV_ACK_HEADER) ) {
          currentMsgType = MT_NAV_ACK;
          payloadSize = sizeof(NAV_ACK);
          ubxMessage.navAck.cls=ubxMessage.navDummy.cls;
          ubxMessage.navAck.id=ubxMessage.navDummy.id;
          //Serial.println("NAV_ACK\n");
        }
        else if ( compareMsgHeader(NAV_NACK_HEADER) ) {
          currentMsgType = MT_NAV_NACK;
          payloadSize = sizeof(NAV_NACK);
          ubxMessage.navNack.cls=ubxMessage.navDummy.cls;
          ubxMessage.navNack.id=ubxMessage.navDummy.id;
          //Serial.println("NAV_NACK\n");
        }
        else if ( compareMsgHeader(NAV_SAT_HEADER) ) {
          currentMsgType = MT_NAV_SAT;
          ubxMessage.navSat.cls=ubxMessage.navDummy.cls;
          ubxMessage.navSat.id=ubxMessage.navDummy.id;
          //Serial.println("NAV_SAT\n");
        }
        else if ( compareMsgHeader(NAV_ID_HEADER) ) {
          currentMsgType = MT_NAV_ID;
          ubxMessage.ubxId.cls=ubxMessage.navDummy.cls;
          ubxMessage.ubxId.id=ubxMessage.navDummy.id;
          //Serial.println("NAV_ID\n");
        }
        else {
          // unknown message type, bail
          currentMsgType = MT_NONE;
          fpos = 0;
          continue;
        }
      }
      if (((fpos-2) < payloadSize)&&(fpos>=4)){
        if(currentMsgType==MT_NAV_PVT) {((unsigned char*)(&ubxMessage.navPvt))[fpos-2] = c;} 
        if(currentMsgType==MT_NAV_DOP) {((unsigned char*)(&ubxMessage.navDOP))[fpos-2] = c;} 
        if(currentMsgType==MT_MON_GNSS) {((unsigned char*)(&ubxMessage.monGNSS))[fpos-2] = c;} 
        if(currentMsgType==MT_MON_VER) {((unsigned char*)(&ubxMessage.monVER))[fpos-2] = c;} 
        if(currentMsgType==MT_NAV_ACK) {((unsigned char*)(&ubxMessage.navAck))[fpos-2] = c;} 
        if(currentMsgType==MT_NAV_NACK) {((unsigned char*)(&ubxMessage.navNack))[fpos-2] = c;} 
        if(currentMsgType==MT_NAV_SAT) {((unsigned char*)(&ubxMessage.navSat))[fpos-2] = c;}
        if(currentMsgType==MT_NAV_ID) {((unsigned char*)(&ubxMessage.ubxId))[fpos-2] = c;}  
      }
       if (fpos==6){
        if(currentMsgType==MT_NAV_PVT){ubxMessage.navPvt.len=payloadSize-6;}//safety if .len is wrong
        if(currentMsgType==MT_NAV_DOP){ubxMessage.navDOP.len=payloadSize-6;}//safety if .len is wrong
        if(currentMsgType==MT_NAV_ID){ payloadSize=ubxMessage.ubxId.len+6;}// .len = 9 bytes for M8, but 10 bytes for M10
        if(currentMsgType==MT_MON_VER){
            if(ubxMessage.monVER.len+6<sizeof(ubxMessage.monVER)){
                payloadSize=ubxMessage.monVER.len+6;
                }//M10 has extensions ??
            else{fpos=0;}//something went wrong, start over again !!!
            }          
        if(currentMsgType==MT_NAV_SAT){
            if(ubxMessage.navSat.len+6<sizeof(ubxMessage.navSat)){//safety if .len is wrong
                payloadSize=ubxMessage.navSat.len+6;
                }//payload is variable with nav_sat msg
            else{fpos=0;}//something went wrong, start over again !!!
            }
      }
      fpos++;
      if ( fpos == (payloadSize) ) {//was (payloadSize+2)
      // All payload bytes have now been received, so we can calculate the 
      // expected checksum value to compare with the next two incoming bytes.
      // checksum has to calculated out of the correct substructure !!!
        calcChecksum(checksum,currentMsgType,payloadSize-2);//was payload !!
      }
      else if ( fpos == (payloadSize+1) ) {//was (payloadSize+3)   fpos-3=c, of payloadsize+1-3=c, dus payloadsize-2
        // First byte after the payload, ie. first byte of the checksum.
        // Does it match the first byte of the checksum we calculated?

        if ( c != checksum[0] ) {
          // Checksum doesn't match, reset to beginning state and try again.
           Serial.println("CkA NIO");
         if ((Time_Set_OK==true)&&(nav_pvt_message>10)){
              char tekst[32] = "";
              sprintf(tekst, "ChecksumA_NIO @ %d\n", (nav_pvt_message-11));
              logERR(tekst);
            }
          fpos = 0; 
        }
      }
      else if ( fpos == (payloadSize+2) ) {//was (payloadSize+4)  fpos-4=c, of payloadsize+1-4=c, dus payloadsize-1
        // Second byte after the payload, ie. second byte of the checksum.
        // Does it match the second byte of the checksum we calculated?
        fpos = 0; // We will reset the state regardless of whether the checksum matches.
        if ( c == checksum[1] ) {
          // Checksum matches, we have a valid message.
          /* iTow has a 18 s diff with UTC, issue with GPS Results !!
           if(currentMsgType==MT_NAV_SAT){
              ubxMessage.navSat.iTOW=ubxMessage.navSat.iTOW-18*1000;//to match 18s diff UTC nav pvt & GPS nav sat !!!
              calcChecksum(checksum,currentMsgType,payloadSize-2);//have to calculate new checksum !!
              ((unsigned char*)(&ubxMessage.navSat))[payloadSize-2]=checksum[0];//checksum is not on a fixed pos, depends from the payload !!!
              ((unsigned char*)(&ubxMessage.navSat))[payloadSize-1]=checksum[1];//checksum is not on a fixed pos, depends from the payload !!!
            }
          */ 
          return currentMsgType; 
        }
        else{ if ((Time_Set_OK==true)&&(nav_pvt_message>10)){
              Serial.println("CkB NIO");
              char tekst[32] = "";
              sprintf(tekst, "ChecksumB_NIO @ %d\n", (nav_pvt_message-11));
              logERR(tekst);
              }
            }
      }    
      else if ( fpos > (payloadSize+2) ) {//was (payloadSize+4)
        // We have now read more bytes than both the expected payload and checksum 
        // together, so something went wrong. Reset to beginning state and try again.
        fpos = 0;
      }
    }
  }
  return MT_NONE;
}



bool Check_ublox_M10()
{
    // Assume Serial2 is already configured to the expected baud
    Serial.println("Checking u-blox M10...");

    for (int i = 0; i < sizeof(UBX_MON_VER); i++) {
        Serial2.write(pgm_read_byte(UBX_MON_VER + i));
    }

    Ublox_serial2(500);

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
    for(int i = 0; i < sizeof(UBX_M10_GET_NAV_RATE); i++) {                        
    Serial2.write( pgm_read_byte(UBX_M10_GET_NAV_RATE+i) );
    }
  Nav_rate_NACK = false;
  High_nav_rate_ACK = false;      
  Ublox_serial2(500);
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
  for(int i = 0; i < sizeof(UBX_M10_SET_HIGH_NAV_RATE); i++) {                        
     Serial2.write( pgm_read_byte(UBX_M10_SET_HIGH_NAV_RATE+i) );
     }
  Ublox_serial2(500); 
  config.ublox_type= UBLOX_TYPE_UNKNOWN;
  config.M10_high_nav= M10_HIGH_NAV_RATE;
  EEPROM.writeByte(0,UBLOX_TYPE_UNKNOWN);
  EEPROM.writeByte(1,M10_HIGH_NAV_RATE); EEPROM.commit();//always set EEPROM to default nav rate.....
  return config.M10_high_nav;
}  
