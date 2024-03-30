#ifndef _TCP_SERVER_H_
#define _TCP_SERVER_H_

#include "Acceptor.h"
#include "InetAddress.h"
#include "UsageEnvironment.h"
#include "../base/New.h"
#include "../base/Logging.h"

#include <assert.h>

class TcpServer {
public:
    virtual ~TcpServer() { Delete::release(mAcceptor); }
    void start() { mAcceptor->listen(); }

protected:
    TcpServer(UsageEnvironment *usageEnvironment, const Ipv4Address &addr) :
        mUsageEnvironment(usageEnvironment),
        mAddr(addr)
    {
        mAcceptor = Acceptor::createNew(usageEnvironment, addr);
        assert(mAcceptor);
        mAcceptor->setNewConnectionCallback(newConnectionCallback, this);
    }

    virtual void handleNewConnection(int connfd) = 0;

private:
    static void newConnectionCallback(void *arg, int connfd)
    {
        TcpServer *tcpServer = (TcpServer *)arg;
        tcpServer->handleNewConnection(connfd);
    }

protected:
    UsageEnvironment *mUsageEnvironment;
    Ipv4Address mAddr;
    Acceptor *mAcceptor;
};

#endif