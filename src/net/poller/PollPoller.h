#ifndef _POLLPOLLER_H_
#define _POLLPOLLER_H_

#include "Poller.h"

#include <vector>
#include <map>
#include <poll.h>

class PollPoller : public Poller
{
public:
    static PollPoller* createNew();

    PollPoller();
    virtual ~PollPoller();

    virtual bool addIOEvent(IOEvent* event);
    virtual bool updateIOEvent(IOEvent* event);
    virtual bool removeIOEvent(IOEvent* event);
    virtual void handleEvent();

private:
    typedef std::vector<struct pollfd> PollFdList;
    PollFdList mPollFdList;
    typedef std::map<int, int> PollFdMap;
    PollFdMap mPollFdMap;
    std::vector<IOEvent*> mEvents;
};

#endif