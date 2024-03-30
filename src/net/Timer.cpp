#include "Timer.h"
#include "../base/New.h"
#include "../base/Logging.h"

#include <sys/timerfd.h>

static int timerfdCreate(int clockid, int flags)
{
    return timerfd_create(clockid, flags);
}

static int timerfdSetTime(int timerfd, Timer::Timestamp timestamp,
                          Timer::TimeInterval timeInterval)
{
    struct itimerspec newVal;
    newVal.it_value.tv_sec = timestamp / 1000;
    newVal.it_value.tv_nsec = timestamp % 1000 * 1000 * 1000;
    newVal.it_interval.tv_sec = timeInterval / 1000;
    newVal.it_interval.tv_nsec = timeInterval % 1000 * 1000 * 1000;

    if(timerfd_settime(timerfd, TFD_TIMER_ABSTIME, &newVal, nullptr) < 0)
        return false;

    return true;
}

Timer::Timestamp Timer::getCurTime()
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (now.tv_sec * 1000 + now.tv_nsec / 1000 / 1000);
}

Timer::Timer(TimerEvent *event, Timer::Timestamp timestamp,
             Timer::TimeInterval timeInterval) :
    mTimerEvent(event),
    mTimestamp(timestamp),
    mTimeInterval(timeInterval)
{
    mIsRepeat = (timeInterval > 0) ? true : false;
}

Timer::~Timer()
{

}

void Timer::handleEvent()
{
    if(mTimerEvent)
        mTimerEvent->handleEvent();
}

TimerManager *TimerManager::createNew(Poller *poller)
{
    if(!poller)
        return nullptr;
    
    int timerfd = timerfdCreate(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if(timerfd < 0) {
        LOG_ERROR("failed to create timer fd\n");
        return nullptr;
    }

    return New<TimerManager>::allocate(timerfd, poller);
}

TimerManager::TimerManager(int timerfd, Poller *poller) :
    mTimerfd(timerfd),
    mPoller(poller),
    mLastTimerId(0)
{
    mTimerIOEvent = IOEvent::createNew(timerfd, this);
    mTimerIOEvent->setReadCallback(handleRead);
    mTimerIOEvent->enableReadHandling();
    modifyTimeout();
    poller->addIOEvent(mTimerIOEvent);
}

TimerManager::~TimerManager()
{
    mPoller->removeIOEvent(mTimerIOEvent);
    Delete::release(mTimerIOEvent);
}

Timer::TimerId TimerManager::addTimer(TimerEvent *event, Timer::Timestamp timestamp,
                        Timer::TimeInterval timeInterval)
{
    Timer timer(event, timestamp, timeInterval);
    mLastTimerId++;
    mTimers.insert(std::make_pair(mLastTimerId, timer));
    mEvents.insert(std::make_pair(TimerIndex(timestamp, mLastTimerId), timer));

    modifyTimeout();

    return mLastTimerId;
}

bool TimerManager::removeTimer(Timer::TimerId timerId)
{
    std::map<Timer::TimerId, Timer>::iterator it = mTimers.find(timerId);
    if(it != mTimers.end()) {
        Timer::Timestamp timestamp = it->second.mTimestamp;
        mTimers.erase(it);
        mEvents.erase(TimerIndex(timestamp, timerId));
    }

    modifyTimeout();

    return true;
}

void TimerManager::modifyTimeout()
{
    std::multimap<TimerIndex, Timer>::iterator it = mEvents.begin();
    if(it != mEvents.end()) {
        Timer timer = it->second;
        timerfdSetTime(mTimerfd, timer.mTimestamp, timer.mTimeInterval);
    }
    else 
        timerfdSetTime(mTimerfd, 0, 0);
}

void TimerManager::handleEvent()
{
    if(mTimerIOEvent)
        mTimerIOEvent->handleEvent();
}

void TimerManager::handleRead(void *arg)
{
    if(!arg)
        return ;

    TimerManager *timerManager = (TimerManager *)arg;
    timerManager->handleTimerEvent();
}

void TimerManager::handleTimerEvent()
{
    Timer::Timestamp now = Timer::getCurTime();
    while(!mTimers.empty() && mEvents.begin()->first.first <= now) {
        Timer::TimerId timerId = mEvents.begin()->first.second;
        Timer timer = mEvents.begin()->second;

        timer.handleEvent();
        mEvents.erase(mEvents.begin());
        if(timer.mIsRepeat) {
            timer.mTimestamp = now + timer.mTimeInterval;
            mEvents.insert(std::make_pair(TimerIndex(timer.mTimestamp, timerId), timer));
        }
        else
            mTimers.erase(timerId);
    }

    modifyTimeout();
}