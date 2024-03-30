#include "H264FileMediaSource.h"
#include "../base/New.h"
#include "../base/Logging.h"

#include <unistd.h>
#include <fcntl.h>
#include <assert.h>

static inline bool startCode3(uint8_t *buf)
{
    if(buf[0] == 0 && buf[1] == 0 && buf[2] == 1)
        return true;
    else 
        return false;
}

static inline bool startCode4(uint8_t *buf)
{
    if(buf[0] == 0 && buf[1] == 0 && buf[2] == 0 && buf[3] == 1)
        return true;
    else 
        return false;
}

static uint8_t *findNextStartCode(uint8_t *buf, int size)
{
    if(size < 3)
        return nullptr;
    
    for(int i = 0; i < size - 3; i++) {
        if(startCode3(buf) || startCode4(buf))
            return buf;
        
        buf++;
    }

    if(startCode3(buf))
        return buf;
    
    return nullptr;
}

H264FileMediaSource *H264FileMediaSource::createNew(UsageEnvironment *usageEnvironment,
                                                    std::string file)
{
    return New<H264FileMediaSource>::allocate(usageEnvironment, file);
}

H264FileMediaSource::H264FileMediaSource(UsageEnvironment *usageEnvironment,
                                         const std::string &file) :
    MediaSource(usageEnvironment),
    mFile(file)
{
    mFd = ::open(file.c_str(), O_RDONLY);
    assert(mFd > 0);

    setFps(25);

    for(int i = 0; i < FRAME_DEFAULT_NUM; i++)
        mUsageEnvironment->threadPool()->addTask(mTask);
}

H264FileMediaSource::~H264FileMediaSource()
{
    ::close(mFd);
}

static int count = 0;

void H264FileMediaSource::readFrame()
{
    MutexLockGuard lockInput(mMutexInput);

    if(mMyAVFrameInputQueue.empty())
        return;
    
    MyAVFrame *frame = mMyAVFrameInputQueue.front();
    frame->mFrameSize = getFrameFromH264File(mFd, frame->mBuffer, FRAME_MAX_SIZE);
    if(frame->mFrameSize < 0)
        return ;

    if(startCode3(frame->mBuffer)) {
        frame->mFrame = frame->mBuffer + 3;
        frame->mFrameSize -= 3;
    }
    else {
        frame->mFrame = frame->mBuffer + 4;
        frame->mFrameSize -= 4;
    }
    
    MutexLockGuard lockOutput(mMutexOutput);
    mMyAVFrameInputQueue.pop();
    mMyAVFrameOutputQueue.push(frame);
}

int H264FileMediaSource::getFrameFromH264File(int fd, uint8_t *frame, int size)
{
    int rSize, frameSize;
    uint8_t *nextStartCode;

    if(fd < 0)
        return -1;
    
    rSize = ::read(fd, frame, size);
    if(rSize < 0)
        return -1;
    if(!startCode3(frame) && !startCode4(frame))
        return -1;
    
    nextStartCode = findNextStartCode(frame + 3, rSize - 3);
    if(!nextStartCode) {
        frameSize = rSize;
        lseek(mFd, 0, SEEK_SET);
    }
    else {
        frameSize = (nextStartCode - frame);
        lseek(mFd, frameSize - rSize, SEEK_CUR);
    }

    return frameSize;
}