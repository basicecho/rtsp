#ifndef _MEDIA_SOURCE_H_
#define _MEDIA_SOURCE_H_

#include "../base/Mutex.h"
#include "UsageEnvironment.h"

#include <stdint.h>
#include <queue>

#define FRAME_MAX_SIZE (1024 * 500)
#define FRAME_DEFAULT_NUM 5

class MediaSource { 
public:
    class MyAVFrame {
    public:
        MyAVFrame() :
            mBuffer(new uint8_t[FRAME_MAX_SIZE]),
            mFrameSize(0)
        { }

        ~MyAVFrame() { delete[] mBuffer; }
    public:
        uint8_t *mBuffer;
        uint8_t *mFrame;
        int mFrameSize;
    };

public:
    virtual ~MediaSource();

    MyAVFrame *getFrame();
    void putFrame(MyAVFrame *frame);
    int getFps() const { return mFps; }

protected:
    MediaSource(UsageEnvironment *usageEnvironment);
    virtual void readFrame() = 0;
    void setFps(int fps) { mFps = fps; }

private:
    static void taskCallback(void *arg);    

protected:
    UsageEnvironment *mUsageEnvironment;
    MyAVFrame mMyAVFrames[FRAME_DEFAULT_NUM];
    Mutex *mMutexInput;
    Mutex *mMutexOutput;
    std::queue<MyAVFrame *> mMyAVFrameInputQueue;
    std::queue<MyAVFrame *> mMyAVFrameOutputQueue;
    ThreadPool::Task mTask;
    int mFps;
};

#endif