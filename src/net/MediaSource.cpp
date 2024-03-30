#include "MediaSource.h"
#include "../base/New.h"

MediaSource::MediaSource(UsageEnvironment *usageEnvironment) :
    mUsageEnvironment(usageEnvironment)
{
    mMutexInput = Mutex::createNew();
    mMutexOutput = Mutex::createNew();

    for(int i = 0; i < FRAME_DEFAULT_NUM; i++)
        mMyAVFrameInputQueue.push(&mMyAVFrames[i]);
    
    mTask.setTaskCallback(taskCallback, this);
}

MediaSource::~MediaSource() 
{
    Delete::release(mMutexInput);
    Delete::release(mMutexOutput);
}

MediaSource::MyAVFrame *MediaSource::getFrame()
{
    MutexLockGuard lock(mMutexOutput);

    if(mMyAVFrameOutputQueue.empty())
        return nullptr;
    
    MyAVFrame *frame = mMyAVFrameOutputQueue.front();
    mMyAVFrameOutputQueue.pop();

    return frame;
}

void MediaSource::putFrame(MyAVFrame *frame)
{
    MutexLockGuard lock(mMutexInput);

    mMyAVFrameInputQueue.push(frame);

    mUsageEnvironment->threadPool()->addTask(mTask);
}

void MediaSource::taskCallback(void *arg)
{
    MediaSource *mediaSource = (MediaSource *)arg;
    mediaSource->readFrame();
}