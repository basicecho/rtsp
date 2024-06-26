#ifndef _EPOLLPOLLER_H_
#define _EPOLLPOLLER_H_

#include "Poller.h"

#include <sys/epoll.h>
#include <vector>

class EPollPoller : public Poller
{
public:
    static EPollPoller* createNew();

    EPollPoller();
    virtual ~EPollPoller();

    virtual bool addIOEvent(IOEvent* event);
    virtual bool updateIOEvent(IOEvent* event);
    virtual bool removeIOEvent(IOEvent* event);
    virtual void handleEvent();

private:
    int mEPollFd;

    typedef std::vector<struct epoll_event> EPollEventList;
    EPollEventList mEPollEventList;
    std::vector<IOEvent*> mEvents;
};

#endif