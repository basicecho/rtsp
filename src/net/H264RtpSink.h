#ifndef _H264_RTP_SINK_H_
#define _H264_RTP_SINK_H_

#include "RtpSink.h"
#include "MediaSource.h"
#include "UsageEnvironment.h"

class H264RtpSink : public RtpSink {
public:
    static H264RtpSink *createNew(UsageEnvironment *usageEnvironment,
                                  MediaSource *mediaSource);
    H264RtpSink(UsageEnvironment *usageEnvironment, MediaSource *mediaSource);
    virtual ~H264RtpSink() = default;

    virtual std::string getMediaDescription(uint16_t port);
    virtual std::string getAttribute();

protected:
    virtual void handleFrame(MediaSource::MyAVFrame *frame);

private:
    RtpPacket mRtpPacket;
    int mCloseRate;
    int mFps;
};

#endif