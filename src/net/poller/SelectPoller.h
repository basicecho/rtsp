#ifndef _SELECT_POLLER_H_
#define _SELECT_POLLER_H_

#include "PollPoller.h"

#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

class SelectPoller : public Poller
{
public:
    static SelectPoller* createNew();

    SelectPoller();
    virtual ~SelectPoller();

    virtual bool addIOEvent(IOEvent* event);
    virtual bool updateIOEvent(IOEvent* event);
    virtual bool removeIOEvent(IOEvent* event);
    virtual void handleEvent();

private:
    fd_set mReadSet;
    fd_set mWriteSet;
    fd_set mExceptionSet;
    int mMaxNumSockets;
    std::vector<IOEvent*> mEvents;
};

#endif