#ifndef _RTP_SINK_H_
#define _RTP_SINK_H_

#include "MediaSource.h"
#include "UsageEnvironment.h"
#include "Rtp.h"
#include "Event.h"

#include <functional>
#include <string>

class RtpSink {
public: 
    using SendPacketCallback = std::function<void(void *, void *, RtpPacket*)>;

    RtpSink(UsageEnvironment *usageEnvironment, MediaSource *mediaSource, int payloadType);
    virtual ~RtpSink();

    virtual std::string getMediaDescription(uint16_t port) = 0;
    virtual std::string getAttribute() = 0;

    void setSendPacketCallback(SendPacketCallback cb, void *arg1, void *arg2);

protected:
    virtual void handleFrame(MediaSource::MyAVFrame *frame) = 0;
    void sendRtpPacket(RtpPacket *packet);
    void start(int ms);
    void stop(); 

private:
    static void timeoutCallback(void *);

protected:
    UsageEnvironment *mUsageEnvironment;
    MediaSource *mMediaSource;
    SendPacketCallback mSendPacketCallback;
    void *mArg1;
    void *mArg2;

    uint8_t mCsrcLen;
    uint8_t mExtension;
    uint8_t mPadding;
    uint8_t mVersion;

    uint8_t mPayloadType;
    uint8_t mMarker;
    uint16_t mSeq;
    uint32_t mTimestamp;
    uint32_t mSSRC;

    // uint8_t mCsrcLen;
    // uint8_t mExtesion;
    // uint8_t mPadding;
    // uint8_t mVersion;

    // uint8_t mPayloadType;
    // uint8_t mMarker;

    // uint16_t mSeq;

    // uint32_t mTimestamp;
    // uint32_t CSRC;
private:
    Timer::TimerId mTimerId;
    TimerEvent *mTimerEvent;
};

#endif