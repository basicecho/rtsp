#ifndef _USAGE_ENVIRONMENT_H_
#define _USAGE_ENVIRONMENT_H_

#include "EventScheduler.h"
#include "../base/ThreadPool.h"

class UsageEnvironment {
public:
    static UsageEnvironment *createNew(EventScheduler *scheduler, ThreadPool *threadPool)
    {
        if(!scheduler)
            return nullptr;

        return New<UsageEnvironment>::allocate(scheduler, threadPool);
    }

    UsageEnvironment(EventScheduler *eventScheduler, ThreadPool *threadPool) :
        mEventScheduler(eventScheduler),
        mThreadPool(threadPool)
    { }

    ~UsageEnvironment() = default;

    EventScheduler *scheduler() { return mEventScheduler; }
    ThreadPool *threadPool() { return mThreadPool; }

private:
    EventScheduler *mEventScheduler;
    ThreadPool *mThreadPool;
};

#endif