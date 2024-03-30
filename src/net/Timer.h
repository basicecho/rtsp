#ifndef _TIMER_H_
#define _TIMER_H_

#include "Event.h"
#include "poller/Poller.h"

#include <stdint.h>
#include <map>

class Timer {
public:
    using TimerId = uint32_t;
    using Timestamp = uint64_t;
    using TimeInterval = int32_t;

    static Timestamp getCurTime();
    ~Timer();

private:
    friend class TimerManager;
    Timer(TimerEvent *event, Timestamp timestamp, TimeInterval timeInterval);
    void handleEvent();

private:
    TimerEvent *mTimerEvent;
    Timestamp mTimestamp;
    TimeInterval mTimeInterval;
    bool mIsRepeat;
};

class TimerManager{
public:
    static TimerManager *createNew(Poller *poller);
    TimerManager(int timerfd, Poller *poller);
    ~TimerManager();

    Timer::TimerId addTimer(TimerEvent *event, Timer::Timestamp timestamp, 
                            Timer::TimeInterval timeInterval);
    bool removeTimer(Timer::TimerId timerId);

private:
    void modifyTimeout();
    void handleEvent();
    static void handleRead(void *arg);
    void handleTimerEvent();

private:
    int mTimerfd;
    Poller *mPoller;
    std::map<Timer::TimerId, Timer> mTimers;

    using TimerIndex = std::pair<Timer::Timestamp, Timer::TimerId>;
    std::multimap<TimerIndex, Timer> mEvents;
    int mLastTimerId;
    IOEvent *mTimerIOEvent;
};

#endif