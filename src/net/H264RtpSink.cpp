#include "H264RtpSink.h"
#include "../base/New.h"
#include "../base/Logging.h"

H264RtpSink *H264RtpSink::createNew(UsageEnvironment *usageEnvironment,
                                    MediaSource *mediaSource)
{
    if(!mediaSource)
        return NULL;

    return New<H264RtpSink>::allocate(usageEnvironment, mediaSource);
}

H264RtpSink::H264RtpSink(UsageEnvironment *usageEnvironment,
                         MediaSource *mediaSource) :
    RtpSink(usageEnvironment, mediaSource, RTP_PAYLOAD_TYPE_H264),
    mCloseRate(90000),
    mFps(mediaSource->getFps())
{
    start(1000 / mFps);
}

std::string H264RtpSink::getMediaDescription(uint16_t port)
{
    char buf[100] = { 0 };
    sprintf(buf, "m=video %hu RTP/AVP %d", port, mPayloadType);

    return std::string(buf);
}

std::string H264RtpSink::getAttribute()
{
    char buf[100] = { 0 };
    sprintf(buf, "a=rtpmap:%d H264/%d\r\n", mPayloadType, mCloseRate);
    sprintf(buf + strlen(buf), "a=framerate:%d", mFps);

    return std::string(buf);
}

void H264RtpSink::handleFrame(MediaSource::MyAVFrame *frame)
{
    RtpHeader *rtpHeader = mRtpPacket.mRtpHeader;
    uint8_t naluType = frame->mFrame[0];

    if(frame->mFrameSize <= RTP_MAX_PKT_SIZE) {
        memcpy(rtpHeader->payload, frame->mFrame, frame->mFrameSize);
        mRtpPacket.mSize = frame->mFrameSize;
        sendRtpPacket(&mRtpPacket);
        mSeq++;

        if(((naluType & 0x1F) == 7) || ((naluType & 0x1F) == 8))
            return ;
    }
    else {
        int pktNum = frame->mFrameSize / RTP_MAX_PKT_SIZE;
        int remainPktSize = frame->mFrameSize % RTP_MAX_PKT_SIZE;
        int pos = 1;

        for(int i = 0; i < pktNum; i++) {
            rtpHeader->payload[0] = (naluType & 0x60) | 28;
            rtpHeader->payload[1] = (naluType & 0x1F);

            if(i == 0)
                rtpHeader->payload[1] |= 0x80;
            else if(i == pktNum - 1 && remainPktSize == 0)
                rtpHeader->payload[1] |= 0x40;

            memcpy(rtpHeader->payload + 2, frame->mFrame + pos, RTP_MAX_PKT_SIZE);
            mRtpPacket.mSize = RTP_MAX_PKT_SIZE + 2;
            sendRtpPacket(&mRtpPacket);

            mSeq++;
            pos += RTP_MAX_PKT_SIZE; 
        }

        if(remainPktSize > 0) {
            rtpHeader->payload[0] = (naluType & 0x60) | 28;
            rtpHeader->payload[1] = (naluType & 0x1F);
            rtpHeader->payload[1] |= 0x40;

            memcpy(rtpHeader->payload + 2, frame->mFrame+ pos, remainPktSize);
            mRtpPacket.mSize = remainPktSize + 2;
            sendRtpPacket(&mRtpPacket);

            mSeq++;
        }
    }
    mTimestamp += mCloseRate / mFps;
}