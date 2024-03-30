#ifndef _ACCEPTOR_H_
#define _ACCEPTOR_H_

#include "Event.h"
#include "InetAddress.h"
#include "TcpSocket.h"
#include "UsageEnvironment.h"

#include <functional>

class Acceptor {
public:
    using NewConnectionCallback = std::function<void(void *, int)>;

    static Acceptor *createNew(UsageEnvironment *usageEnvironment,
                               const Ipv4Address &addr);

    Acceptor(UsageEnvironment *usageEnvironment, const Ipv4Address &addr);
    ~Acceptor();

    bool listening() { return mListenning; }
    void listen();
    void setNewConnectionCallback(NewConnectionCallback cb, void *arg);

private:
    static void readCallback(void *arg);
    void handleRead();

private:
    UsageEnvironment *mUsageEnvironment;
    IOEvent *mAcceptIOEvent;
    TcpSocket mSocket;
    Ipv4Address mAddr;
    bool mListenning;

    NewConnectionCallback mNewConnectionCallback;
    void *mArg;
};

#endif