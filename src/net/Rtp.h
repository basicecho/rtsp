#ifndef _RTP_H_
#define _RTP_H_

#include <stdint.h>
#include <stdlib.h>

#define RTP_VERSION 2

#define RTP_PAYLOAD_TYPE_H264 96
#define RTP_PAYLOAD_TYPE_AAC  97

#define RTP_HEADER_SIZE 12
#define RTP_MAX_PKT_SIZE 1400

struct RtpHeader {
    uint8_t csrcLen: 4;
    uint8_t extension: 1;
    uint8_t padding: 1;
    uint8_t version: 2;

    uint8_t payloadType: 7;
    uint8_t marker: 1;

    uint16_t seq;
    uint32_t timestamp;
    uint32_t ssrc;

    uint8_t payload[0];
};

class RtpPacket {
public:
    RtpPacket() :
        mBufferTcp(new uint8_t[RTP_HEADER_SIZE + RTP_MAX_PKT_SIZE + 100]),
        mBufferUdp(mBufferTcp + 4),
        mRtpHeader((RtpHeader *)mBufferUdp),
        mSize(0)
    { }

    ~RtpPacket() { delete mBufferTcp; }

    uint8_t *mBufferTcp;
    uint8_t *mBufferUdp;
    struct RtpHeader * const mRtpHeader;
    int mSize;
};

#endif