#ifndef _EVENT_SCHEDULER_H_
#define _EVENT_SCHEDULER_H_

#include "poller/Poller.h"
#include "Timer.h"
#include "../base/Mutex.h"

#include <vector>
#include <queue>
#include <functional>

class EventScheduler {
public:
    enum PollerType {
        POLLER_SELECT,
        POLLER_POLL,
        POLLER_EPOLL
    };

    using Callback = std::function<void(void *)>;

public:
    static EventScheduler *createNew(PollerType type);

    EventScheduler(PollerType type, int fd);
    ~EventScheduler();

    void addTriggerEvent(TriggerEvent *event) { mTriggerEvents.push_back(event); }

    Timer::TimerId addTimedEventAfter(TimerEvent *event, Timer::TimeInterval delay)
    {
        Timer::Timestamp when = Timer::getCurTime();
        when += delay;
        return mTimerManager->addTimer(event, when, 0);
    }

    Timer::TimerId addTimedEventAt(TimerEvent *event, Timer::Timestamp when)
    {
        return mTimerManager->addTimer(event, when, 0);
    }

    Timer::TimerId addTimedEventEvery(TimerEvent *event, Timer::TimeInterval interval)
    {
        Timer::Timestamp when = Timer::getCurTime();
        when += interval;

        return mTimerManager->addTimer(event, when, interval);
    }

    bool removeTimerEvent(Timer::TimerId timerId) { return mTimerManager->removeTimer(timerId); }

    bool addIOEvent(IOEvent *event) 
    { 
        mPoller->addIOEvent(event);
        return true; 
    }
    
    bool updateIOEvent(IOEvent *event) 
    {
        mPoller->updateIOEvent(event);
        return true;
    }

    bool removeIOEvent(IOEvent *event) 
    { 
        mPoller->removeIOEvent(event);
        return true;
    }

    void loop();
    void wakeup();

    // 想当前线程添加事件，并运行
    void runInLocalThread(Callback callback, void *arg);
    void handleOtherEvent();

private:
    void handleTriggerEvents();
    static void handleReadCallback(void *);
    void handleRead();

private:
    Poller *mPoller;
    bool mQuit;
    TimerManager *mTimerManager;
    std::vector<TriggerEvent *> mTriggerEvents; 

    // 这个唤醒事件，暂时是没什么用的
    int mWakeupFd;
    IOEvent *mWakeIOEvent;

    std::queue<std::pair<Callback, void *>> mCallbackQueue;
    Mutex *mMutex;
};

#endif