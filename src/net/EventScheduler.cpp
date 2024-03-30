#include "EventScheduler.h"
#include "../base/New.h"
#include "../base/Logging.h"
#include "poller/SelectPoller.h"
#include "poller/PollPoller.h"
#include "poller/EPollPoller.h"

#include <sys/eventfd.h>

static int createEventfd()
{
    int eventfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if(eventfd < 0) {
        LOG_ERROR("failed to create event fd\n");
        return -1;
    }

    return eventfd;
}

EventScheduler *EventScheduler::createNew(PollerType type)
{
    if(type != POLLER_SELECT && type != POLLER_POLL && type != POLLER_EPOLL)
        return nullptr;
    
    int eventfd = createEventfd();
    if(eventfd < 0)
        return nullptr;
    
    return New<EventScheduler>::allocate(type, eventfd);
}

EventScheduler::EventScheduler(PollerType type, int fd) :
    mWakeupFd(fd),
    mQuit(false)
{
    switch(type) {
    case POLLER_SELECT:
        mPoller = SelectPoller::createNew();
        break;
    
    case POLLER_POLL:
        mPoller = PollPoller::createNew();
        break;

    case POLLER_EPOLL:
        mPoller = EPollPoller::createNew();
        break;

    default:
        _exit(-1);
        break;
    }

    mTimerManager = TimerManager::createNew(mPoller);

    mWakeIOEvent = IOEvent::createNew(mWakeupFd, this);
    mWakeIOEvent->setReadCallback(handleReadCallback);
    mWakeIOEvent->enableReadHandling();
    mPoller->addIOEvent(mWakeIOEvent);

    mMutex = Mutex::createNew();
}

EventScheduler::~EventScheduler()
{
    mPoller->removeIOEvent(mWakeIOEvent);
    ::close(mWakeupFd);

    Delete::release(mWakeIOEvent);
    Delete::release(mTimerManager);
    Delete::release(mPoller);
    Delete::release(mMutex);
}

void EventScheduler::loop()
{
    while(!mQuit) {
        this->handleTriggerEvents();
        mPoller->handleEvent();
        this->handleOtherEvent();
    }
}

void EventScheduler::wakeup()
{
    uint64_t on = 1;
    ::write(mWakeupFd, &on, sizeof(on));
}

void EventScheduler::runInLocalThread(Callback callback, void *arg)
{
    MutexLockGuard lock(mMutex);
    mCallbackQueue.push(std::make_pair(callback, arg));
}

void EventScheduler::handleOtherEvent()
{
    MutexLockGuard lock(mMutex);
    while(!mCallbackQueue.empty()) {
        std::pair<Callback, void *> event = mCallbackQueue.front();
        event.first(event.second);
    }
}

void EventScheduler::handleTriggerEvents()
{
    for(std::vector<TriggerEvent *>::iterator it = mTriggerEvents.begin();
        it != mTriggerEvents.end(); it++) {
        (*it)->handleEvent();
    }

    mTriggerEvents.clear();
}

void EventScheduler::handleReadCallback(void *arg)
{
    EventScheduler *scheduler = (EventScheduler *)arg;
    scheduler->handleRead();
}

void EventScheduler::handleRead()
{
    uint64_t on;
    while(::read(mWakeupFd, &on, sizeof(on)) > 0);
}