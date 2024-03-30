#include "RtpSink.h"
#include "../base/New.h"
#include "../base/Logging.h"

#include <arpa/inet.h>

RtpSink::RtpSink(UsageEnvironment *usageEnvironment, MediaSource *mediaSource,
                 int payloadType) :
    mUsageEnvironment(usageEnvironment),
    mMediaSource(mediaSource),
    mSendPacketCallback(nullptr),
    mCsrcLen(0),
    mExtension(0),
    mPadding(0),
    mVersion(RTP_VERSION),
    mPayloadType(payloadType),
    mMarker(0),
    mSeq(0), 
    mTimestamp(0),
    mTimerId(0)
{
    mSSRC = rand();

    mTimerEvent = TimerEvent::createNew(this);
    mTimerEvent->setTimeoutCallback(timeoutCallback);
}

RtpSink::~RtpSink()
{
    mUsageEnvironment->scheduler()->removeTimerEvent(mTimerId);
    Delete::release(mTimerEvent);
}

void RtpSink::setSendPacketCallback(SendPacketCallback cb, void *arg1, void *arg2)
{
    mSendPacketCallback = cb;
    mArg1 = arg1;
    mArg2 = arg2;
}

void RtpSink::sendRtpPacket(RtpPacket *packet)
{
    RtpHeader* rtpHead = packet->mRtpHeader;
    rtpHead->csrcLen = mCsrcLen;
    rtpHead->extension = mExtension;
    rtpHead->padding = mPadding;
    rtpHead->version = mVersion;
    rtpHead->payloadType = mPayloadType;
    rtpHead->marker = mMarker;
    rtpHead->seq = htons(mSeq);
    rtpHead->timestamp = htonl(mTimestamp);
    rtpHead->ssrc = htonl(mSSRC);
    packet->mSize += RTP_HEADER_SIZE;
    
    if(mSendPacketCallback)
        mSendPacketCallback(mArg1, mArg2, packet);
}

void RtpSink::start(int ms)
{
    mTimerId = mUsageEnvironment->scheduler()->addTimedEventEvery(mTimerEvent, ms);
}

void RtpSink::stop()
{
    mUsageEnvironment->scheduler()->removeTimerEvent(mTimerId);
}

void RtpSink::timeoutCallback(void* arg)
{
    RtpSink* rtpSink = (RtpSink*)arg;
    MediaSource::MyAVFrame* frame = rtpSink->mMediaSource->getFrame();
    if(!frame)
    {
        return;
    }

    rtpSink->handleFrame(frame);

    rtpSink->mMediaSource->putFrame(frame);
}