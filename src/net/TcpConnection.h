#ifndef _TCP_CONNECTION_H_
#define _TCP_CONNECTION_H_

#include "Event.h"
#include "Buffer.h"
#include "TcpSocket.h"
#include "UsageEnvironment.h"

#include  <functional>

class TcpConnection {
public:
    using DisConnectionCallback = std::function<void(void *, int)>;

    TcpConnection(UsageEnvironment *usageEnvironment, int sockfd);
    virtual ~TcpConnection();

    void setDisconnectionCallback(DisConnectionCallback cb, void *arg);

protected:
    void enableReadHandling();
    void enalbeWriteHandling();
    void enableErrorHandling();
    void disableReadHandling();
    void disableWriteHandling();
    void disableErrorHandling();

    virtual void handleRead();
    virtual void handleReadBytes();
    virtual void handleWrite();
    virtual void handleError();

    void handleDisconnection();

private:
    static void readCallback(void *arg);
    static void writeCallback(void *arg);
    static void errorCallback(void *arg);

protected:
    UsageEnvironment *mUsageEnvironment;
    TcpSocket mSocket;
    IOEvent *mTcpConnIOEvent;
    DisConnectionCallback mDisConnectionCallback;
    void *mArg;
    Buffer mInputBuffer;
    Buffer mOutputBuffer;
    char mBuffer[2048];
};

#endif