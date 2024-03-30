#ifndef _EVENT_H_
#define _EVENT_H_

#include "../base/New.h"
#include "../base/Logging.h"

#include <functional>

using EventCallback = std::function<void(void *)>;

class TriggerEvent {
public:
    static TriggerEvent *createNew()
    {
        return New<TriggerEvent>::allocate(nullptr);
    }

    static TriggerEvent *createNew(void *arg)
    {
        return New<TriggerEvent>::allocate(arg);
    }

    TriggerEvent(void *arg) :
        mArg(arg),
        mTriggerCallback(NULL)
    { }

    ~TriggerEvent() = default;

    void setArg(void *arg) { mArg = arg; }
    void setTriggerCallback(EventCallback cb) { mTriggerCallback = cb; }
    
    void handleEvent()
    {
        if(mTriggerCallback)
            mTriggerCallback(mArg);
    }

private:
    void *mArg;
    EventCallback mTriggerCallback;
};

class TimerEvent {
public:
    static TimerEvent *createNew()
    {
        return New<TimerEvent>::allocate(nullptr);
    }

    static TimerEvent *createNew(void *arg)
    {
        return New<TimerEvent>::allocate(arg);
    }

    TimerEvent(void *arg) :
        mArg(arg),
        mTimeoutCallback(nullptr)
    { }

    ~TimerEvent() = default;

    void setArg(void *arg) { mArg = arg; }
    void setTimeoutCallback(EventCallback cb) { mTimeoutCallback = cb; }
    
    void handleEvent()
    {
        if(mTimeoutCallback)
            mTimeoutCallback(mArg);
    }

private:
    void *mArg;
    EventCallback mTimeoutCallback;
};

class IOEvent{
public:
    enum IOEventType {
        EVENT_NONE = 0,
        EVENT_READ = 1,
        EVENT_WRITE = 2,
        EVENT_ERROR = 4
    };

    static IOEvent *createNew(int fd)
    {
        return New<IOEvent>::allocate(fd, nullptr);
    }

    static IOEvent *createNew(int fd, void *arg)
    {
        return New<IOEvent>::allocate(fd, arg);
    }

    IOEvent(int fd, void *arg) :
        mFd(fd),
        mArg(arg)
    { }

    ~IOEvent() = default;

    int getFd() const { return mFd; }
    int getEvent() const { return mEvent; }
    void setREvent(int event) { mREvent = event; }
    void setArg(void *arg) { mArg = arg; }

    void setReadCallback(EventCallback cb) { mReadCallback = cb; }
    void setWriteCallback(EventCallback cb) { mWriteCallback = cb; }
    void setErrorCallback(EventCallback cb) { mErrorCallback = cb; }

    void enableReadHandling() { mEvent |= EVENT_READ; }
    void enableWriteHandling() { mEvent |= EVENT_WRITE; }
    void enableErrorHandling() { mEvent |= EVENT_ERROR; }
    void disableReadHandling() { mEvent &= ~EVENT_READ; }
    void disableWriteHandling() { mEvent &= ~EVENT_WRITE; }
    void disableErrorHandling() { mEvent &= ~EVENT_ERROR; }

    bool isNoneHandling() { return mEvent == EVENT_NONE; }
    bool isReadHandling() { return (mEvent & EVENT_READ) != 0; }
    bool isWriteHandling() { return (mEvent & EVENT_WRITE) != 0; }
    bool isErrorHandling() { return (mEvent & EVENT_ERROR) != 0; }

    // ??????
    void handleEvent() {
        if((mREvent & EVENT_READ) && mReadCallback)
            mReadCallback(mArg);
        if((mREvent & EVENT_WRITE) && mWriteCallback)
            mWriteCallback(mArg);
        if((mREvent & EVENT_ERROR) && mErrorCallback)
            mErrorCallback(mArg);
    }

private:
    int mFd;
    int mREvent;
    int mEvent;

    void *mArg;
    EventCallback mReadCallback;
    EventCallback mWriteCallback;
    EventCallback mErrorCallback;
};

#endif