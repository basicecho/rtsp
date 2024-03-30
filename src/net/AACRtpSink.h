#ifndef _AAC_RTP_SINK_H_
#define _AAC_RTP_SINK_H_

#include "MediaSource.h"
#include "RtpSink.h"
#include "UsageEnvironment.h"

class AACRtpSink : public RtpSink {
public:
    static AACRtpSink *createNew(UsageEnvironment *usageEnvironment,
                          MediaSource *mediaSource);
    AACRtpSink(UsageEnvironment *usageEnvironment, MediaSource *MediaSource,
               int payloadType);
    virtual ~AACRtpSink();
    
    virtual std::string getMediaDescription(uint16_t port);
    virtual std::string getAttribute();

protected:
    void handleFrame(MediaSource::MyAVFrame *frame);

private:
    RtpPacket mRtpPacket;
    uint32_t mSampleRate;
    uint32_t mChannels;
    int mFps;
};

#endif