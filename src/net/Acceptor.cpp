#include "Acceptor.h"
#include "SocketsOps.h"
#include "../base/New.h"
#include "../base/Logging.h"

Acceptor *Acceptor::createNew(UsageEnvironment *usageEnvironment,
                              const Ipv4Address &addr)
{
    return New<Acceptor>::allocate(usageEnvironment, addr);
}

Acceptor::Acceptor(UsageEnvironment *usageEnvironment,
                   const Ipv4Address &addr) :
    mUsageEnvironment(usageEnvironment),
    mSocket(sockets::createTcpSocket()),
    mAddr(addr),
    mListenning(false),
    mNewConnectionCallback(nullptr)
{
    mSocket.setReuseAddr(1);
    mSocket.bind(mAddr);
    mAcceptIOEvent = IOEvent::createNew(mSocket.fd(), this);
    mAcceptIOEvent->setReadCallback(readCallback);
    mAcceptIOEvent->enableReadHandling();
}

Acceptor::~Acceptor()
{
    if(mListenning)
        mUsageEnvironment->scheduler()->removeIOEvent(mAcceptIOEvent);
    
    Delete::release(mAcceptIOEvent);
}

void Acceptor::listen()
{
    mListenning = true;
    mSocket.listen(1024);
    mUsageEnvironment->scheduler()->addIOEvent(mAcceptIOEvent);
}

void Acceptor::setNewConnectionCallback(NewConnectionCallback cb, void *arg)
{
    mNewConnectionCallback = cb;
    mArg = arg;
}

void Acceptor::readCallback(void *arg)
{
    Acceptor *acceptor = (Acceptor *)arg;
    acceptor->handleRead();
}

void Acceptor::handleRead()
{
    int connfd = mSocket.accept();
    LOG_DEBUG("client connect: %d\n", connfd);
    if(mNewConnectionCallback)
        mNewConnectionCallback(mArg, connfd);
}