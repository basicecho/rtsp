#include "AACFileMediaSource.h"
#include "../base/New.h"
#include "../base/Logging.h"

#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>

AACFileMediaSource *AACFileMediaSource::createNew(UsageEnvironment *usageEnvironment,
                                                  std::string file)
{
    return New<AACFileMediaSource>::allocate(usageEnvironment, file);
}

AACFileMediaSource::AACFileMediaSource(UsageEnvironment *usageEnvironment,
                                       const std::string &file) :
    MediaSource(usageEnvironment),
    mFile(file)
{
    mFd = ::open(file.c_str(), O_RDONLY);
    assert(mFd > 0);

    setFps(43);

    for(int i = 0; i < FRAME_DEFAULT_NUM; i++)
        usageEnvironment->threadPool()->addTask(mTask);
}

AACFileMediaSource::~AACFileMediaSource()
{
    ::close(mFd);
}

void AACFileMediaSource::readFrame()
{
    MutexLockGuard lockInput(mMutexInput);

    if(mMyAVFrameInputQueue.empty())
        return ;
    
    // 返回的是总长度
    MyAVFrame *frame = mMyAVFrameInputQueue.front();
    frame->mFrameSize = getFrameFromAACFile(mFd, frame->mBuffer, FRAME_MAX_SIZE);

    if(frame->mFrameSize < 0)
        return ;
    frame->mFrame = frame->mBuffer;

    MutexLockGuard lockOutput(mMutexOutput);
    mMyAVFrameInputQueue.pop();
    mMyAVFrameOutputQueue.push(frame);
}

bool AACFileMediaSource::parseAdtsHeader(uint8_t *in, struct AdtsHeader *res)
{
    if((in[0] == 0xFF) && ((in[1] & 0xF0) == 0xF0)) {
        res->id = ((uint8_t)in[1] & 0x08) >> 3;
        res->layer = ((uint8_t)in[1] & 0x06) >> 1;
        res->protectionAbsent = (uint8_t)in[1] & 0x01;
        res->profile = ((uint8_t)in[2] & 0xC0) >> 6;
        res->samplingFreqIndex = ((uint8_t)in[2] & 0x3C) >> 2;
        res->privateBit = ((uint8_t)in[2] & 0x02) >> 1;
        res->channelCfg = (((uint8_t)in[2] & 0x01) << 2) |
                          (((uint8_t)in[3] & 0xC0) >> 6);
        res->originalCopy = ((uint8_t)in[3] & 0x20) >> 5;
        res->home = ((uint8_t)in[3] & 0x10) >> 4;

        res->copyRightIdentificationBit = ((uint8_t)in[3] & 0x08) >> 3;
        res->copyRightIdentificationStart = ((uint8_t)in[3] & 0x04) >> 2;
        res->aacFrameLength = (((uint8_t)in[3] & 0x03) << 11) |
                              (((uint8_t)in[4] & 0xFF) << 3) |
                              (((uint8_t)in[5] & 0xE0) >> 5);
        res->adtsBufferFullness = (((uint8_t)in[5] & 0x1F) << 6) |
                                  (((uint8_t)in[6] & 0xFC) >> 2);
        res->numberOfRawDataBlockInFrame = ((uint8_t)in[6] & 0x03); 

        return true;
    }
    else {
        LOG_WARNING("aac adts header parse error");
        return false;
    }
}

int AACFileMediaSource::getFrameFromAACFile(int fd, uint8_t *frame, int size)
{
    uint8_t buf[7];
    int ret;

    ret = ::read(fd, buf, 7);
    if(ret <= 0) {
        lseek(fd, 0, SEEK_SET);
        ret = ::read(fd, buf, 7);
        if(ret <= 0)
            return -1;
    }

    if(parseAdtsHeader(buf, &mAdtsHeader) == false) {
        LOG_WARNING("adts parse error\n");
        return -1;
    }

    if(mAdtsHeader.aacFrameLength > size)
        return -1;
    
    // memcpy(frame, buf, 7);
    ret = ::read(fd, frame, mAdtsHeader.aacFrameLength - 7);
    if(ret < 0) {
        LOG_WARNING("adts read data error\n");
        return -1;
    }

    return mAdtsHeader.aacFrameLength - 7;
}