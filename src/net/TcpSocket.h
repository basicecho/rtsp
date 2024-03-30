#ifndef _TCP_SOCKET_H_
#define _TCP_SOCKET_H_

#include "InetAddress.h"
#include "SocketsOps.h"

#include <stdint.h>
#include <string>

class TcpSocket {
public:
    explicit TcpSocket(int sockfd) :
        mSockfd(sockfd)
    { }

    ~TcpSocket() { sockets::close(mSockfd); }
    int fd() { return mSockfd; }
    bool bind(Ipv4Address &addr) { return sockets::bind(mSockfd, addr.getIp(), addr.getPort()); }
    bool listen(int backlog) { return sockets::listen(mSockfd, backlog); }
    int accept() { return sockets::accept(mSockfd); }
    void setReuseAddr(int on) { sockets::setReuseAddr(mSockfd, on); }

private:
    int mSockfd;
};

#endif