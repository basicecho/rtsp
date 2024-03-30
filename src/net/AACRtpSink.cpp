#include "AACRtpSink.h"
#include "../base/New.h"
#include "../base/Logging.h"

static uint32_t AACSampleRate[16] = {
    96000, 88200, 64000, 48000,
    44100, 32000, 24000, 22050,
    16000, 12000, 11025, 8000,
    7305,  0,     0,     0
};

AACRtpSink *AACRtpSink::createNew(UsageEnvironment *usageEnvironment,
                                  MediaSource *mediaSource)
{
    return New<AACRtpSink>::allocate(usageEnvironment, mediaSource, RTP_PAYLOAD_TYPE_AAC);
}

AACRtpSink::AACRtpSink(UsageEnvironment *usageEnvironment, MediaSource *mediaSource,
                       int payloadType) :
    RtpSink(usageEnvironment, mediaSource, payloadType),
    mSampleRate(48000),
    mChannels(2),
    mFps(mediaSource->getFps())
{
    mMarker = 1;
    start(1000 / mFps);
}

AACRtpSink::~AACRtpSink()
{

}

// 总的描述
std::string AACRtpSink::getMediaDescription(uint16_t port)
{
    char buf[100] = { 0 };
    sprintf(buf, "m=audio %hu RTP/AVP %d", port, mPayloadType);

    return std::string(buf);
}

// 附加信息
std::string AACRtpSink::getAttribute()
{
    char buf[500] = { 0 };
    sprintf(buf, "a=rtpmap:97 mpeg4-generic/%u/%u\r\n", mSampleRate, mChannels);

    uint8_t index;
    for(index = 0; index < 16; index++) {
        if(AACSampleRate[index] == mSampleRate)
            break;
    }
    if(index == 16)
        return "";

    uint8_t profile = 1;
    char configStr[10] = { 0 };
    sprintf(configStr, "%02x%02x", (uint8_t)((profile + 1) << 3) | (index >> 1),
            (uint8_t)((index << 7) | (mChannels << 3)));

    for(int i = 0; i < 10; i++) {
        printf("configStr[%d] = %d\n", i, static_cast<int>(configStr[i]));
    }
    printf("\n");

    sprintf(buf + strlen(buf),
            "a=fmtp:%d profile-level-id=1;"
            "mode=AAC-hbr;"
            "sizelength=13;indexlength=3;indexdeltalength=3;"
            "config=%04u",
            mPayloadType,
            atoi(configStr));

    return std::string(buf);
}

void AACRtpSink::handleFrame(MediaSource::MyAVFrame *frame)
{
    RtpHeader *rtpHeader = mRtpPacket.mRtpHeader;
    // int frameSize = frame->mFrameSize - 7;
    int frameSize = frame->mFrameSize;

    rtpHeader->payload[0] = 0x00;
    rtpHeader->payload[1] = 0x10;
    rtpHeader->payload[2] = (frameSize & 0x1FE0) >> 5;
    rtpHeader->payload[3] = (frameSize & 0x1F) << 3;

    // memcpy(rtpHeader->payload + 4, frame->mFrame + 7, frameSize);
    memcpy(rtpHeader->payload + 4, frame->mFrame, frameSize);
    mRtpPacket.mSize = frameSize + 4;

    sendRtpPacket(&mRtpPacket);

    mSeq++;

    mTimestamp += mSampleRate * (1000 / mFps) / 1000;
}