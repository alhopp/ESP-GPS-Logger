#include "Ublox/ublox.h"
#include "Core/Definitions.h"

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

