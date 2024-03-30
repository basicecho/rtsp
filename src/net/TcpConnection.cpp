#include "TcpConnection.h"
#include "SocketsOps.h"
#include "../base/New.h"
#include "../base/Logging.h"

TcpConnection::TcpConnection(UsageEnvironment *usageEnvironment, int sockfd) :
    mUsageEnvironment(usageEnvironment),
    mSocket(sockfd),
    mDisConnectionCallback(nullptr)
{
    mTcpConnIOEvent = IOEvent::createNew(sockfd, this);
    mTcpConnIOEvent->setReadCallback(readCallback);
    mTcpConnIOEvent->setWriteCallback(writeCallback);
    mTcpConnIOEvent->setErrorCallback(errorCallback);
    mTcpConnIOEvent->enableReadHandling();
    mUsageEnvironment->scheduler()->addIOEvent(mTcpConnIOEvent);
}

TcpConnection::~TcpConnection()
{
    mUsageEnvironment->scheduler()->removeIOEvent(mTcpConnIOEvent);
    Delete::release(mTcpConnIOEvent);
}

void TcpConnection::setDisconnectionCallback(DisConnectionCallback cb, void *arg)
{
    mDisConnectionCallback = cb;
    mArg = arg;
}

void TcpConnection::enableReadHandling()
{
    if(mTcpConnIOEvent->isReadHandling())
        return ;
    
    mTcpConnIOEvent->enableWriteHandling();
    mUsageEnvironment->scheduler()->updateIOEvent(mTcpConnIOEvent);
}

void TcpConnection::enalbeWriteHandling()
{
    if(mTcpConnIOEvent->isWriteHandling())
        return ;
    
    mTcpConnIOEvent->enableWriteHandling();
    mUsageEnvironment->scheduler()->updateIOEvent(mTcpConnIOEvent);
}

void TcpConnection::enableErrorHandling()
{
    if(mTcpConnIOEvent->isErrorHandling())
        return ;
    
    mTcpConnIOEvent->enableErrorHandling();
    mUsageEnvironment->scheduler()->updateIOEvent(mTcpConnIOEvent);
}

void TcpConnection::disableReadHandling()
{
    if(mTcpConnIOEvent->isReadHandling() == false)
        return ;
    
    mTcpConnIOEvent->disableReadHandling();
    mUsageEnvironment->scheduler()->updateIOEvent(mTcpConnIOEvent);
}

void TcpConnection::disableWriteHandling()
{
    if(mTcpConnIOEvent->isWriteHandling() == false)
        return ;
    
    mTcpConnIOEvent->disableWriteHandling();
    mUsageEnvironment->scheduler()->updateIOEvent(mTcpConnIOEvent);
}

void TcpConnection::disableErrorHandling()
{
    if(mTcpConnIOEvent->isErrorHandling() == false)
        return ;
    
    mTcpConnIOEvent->disableErrorHandling();
    mUsageEnvironment->scheduler()->updateIOEvent(mTcpConnIOEvent);
}

void TcpConnection::handleRead()
{
    int ret = mInputBuffer.read(mSocket.fd());

    if(ret == 0) {
        LOG_DEBUG("client disconnect\n");
        handleDisconnection();
        return ;
    }
    else if(ret < 0) {
        LOG_ERROR("read error\n");
        return ;
    }

    handleReadBytes();
}

void TcpConnection::handleReadBytes()
{
    LOG_DEBUG("default read handle\n");
    mInputBuffer.retrieveAll();
}

void TcpConnection::handleWrite()
{
    LOG_DEBUG("default write handle\n");
    mInputBuffer.retrieveAll();
}

void TcpConnection::handleError()
{
    LOG_DEBUG("default error handle\n");
}

void TcpConnection::handleDisconnection()
{
    if(mDisConnectionCallback)
        mDisConnectionCallback(mArg, mSocket.fd());
}

void TcpConnection::readCallback(void *arg)
{
    TcpConnection *tcpConnection = (TcpConnection *)arg;
    tcpConnection->handleRead();
}

void TcpConnection::writeCallback(void *arg)
{
    TcpConnection *tcpConnection = (TcpConnection *)arg;
    tcpConnection->handleWrite();
}

void TcpConnection::errorCallback(void *arg)
{
    TcpConnection *tcpConnection = (TcpConnection *)arg;
    tcpConnection->handleError();
}