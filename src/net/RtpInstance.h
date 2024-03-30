#ifndef _RTP_INSTANCE_H_
#define _RTP_INSTANCE_H_

#include "../base/New.h"
#include "InetAddress.h"
#include "SocketsOps.h"
#include "Rtp.h"

/*
创建rtp实例

提供发送的方式
含有会话id
*/

class RtpInstance {
public:
    enum RtpType {
        RTP_OVER_UDP,
        RTP_OVER_TCP
    };

    static RtpInstance *createNewOverUdp(int localSockfd, uint16_t localPort,
                                  std::string destIp, uint16_t destPort)
    {
        return New<RtpInstance>::allocate(localSockfd, localPort, destIp, destPort);
    }

    static RtpInstance *createNewOverTcp(int clientSockfd, uint8_t rtpChannel)
    {
        return New<RtpInstance>::allocate(clientSockfd, rtpChannel);
    }

    ~RtpInstance()
    {
        sockets::close(mSockfd);
    }

    int send(RtpPacket *rtpPacket)
    {
        if(mRtpType == RTP_OVER_UDP)
            return sendOverUdp(rtpPacket->mBufferUdp, rtpPacket->mSize);
        else {
            uint8_t *rtpPktPtr = rtpPacket->mBufferTcp;
            rtpPktPtr[0] = '$';
            rtpPktPtr[1] = mRtpChannel;
            rtpPktPtr[2] = (uint8_t)(((rtpPacket->mSize) & 0xFF00) >> 8);
            rtpPktPtr[3] = (uint8_t)((rtpPacket->mSize) & 0xFF);
            return sendOverTcp(rtpPktPtr, rtpPacket->mSize + 4);
        }
    }

    uint16_t getLocalPort() { return mLocalPort; }
    uint16_t getPeerPort() { return mDestAddr.getPort(); }

    bool alive() { return mIsAlive; }
    void setAlive(bool alive) { mIsAlive = alive; }
    uint16_t sessionId() { return mSessionId; }
    void setSessionId(uint16_t sessionId) { mSessionId = sessionId; }

private:
    int sendOverUdp(void *buf, int size)
    {
        return sockets::sendto(mSockfd, buf, size, mDestAddr.getAddr());
    }

    int sendOverTcp(void *buf, int size)
    {
        return sockets::write(mSockfd, buf, size);
    }

public:
    RtpInstance(int localSockfd, int localPort, const std::string &destIp, uint16_t destPort) :
        mRtpType(RTP_OVER_UDP),
        mSockfd(localSockfd),
        mSessionId(0),
        mIsAlive(false),
        mLocalPort(localPort),
        mDestAddr(destIp, destPort)
    { }

    RtpInstance(int clientSockfd, int rtpChannel) :
        mRtpType(RTP_OVER_TCP),
        mSockfd(clientSockfd),
        mSessionId(0),
        mIsAlive(false),
        mRtpChannel(rtpChannel)
    { }

private:
    RtpType mRtpType;;
    int mSockfd;
    uint16_t mSessionId;
    bool mIsAlive;

    uint16_t mLocalPort;  // 这个不太懂
    Ipv4Address mDestAddr;

    uint8_t mRtpChannel;
};

class RtcpInstance
{
public:
    static RtcpInstance* createNew(int localSockfd, uint16_t localPort,
                                    std::string destIp, uint16_t destPort)
    {
        //return new RtcpInstance(localSockfd, localPort, destIp, destPort);
        return New<RtcpInstance>::allocate(localSockfd, localPort, destIp, destPort);
    }

    ~RtcpInstance()
    {
        sockets::close(mLocalSockfd);
    }

    int send(void* buf, int size)
    {
        return sockets::sendto(mLocalSockfd, buf, size, mDestAddr.getAddr());
    }

    int recv(void* buf, int size, Ipv4Address* addr)
    {
        return 0;
    }

    uint16_t getLocalPort() const { return mLocalPort; }

    bool alive() const { return mIsAlive; return true; }
    bool setAlive(bool alive) { mIsAlive = alive; return true; };
    void setSessionId(uint16_t sessionId) { mSessionId = sessionId; }
    uint16_t sessionId() const { return mSessionId; }

public:
    RtcpInstance(int localSockfd, uint16_t localPort,
                    std::string destIp, uint16_t destPort) :
        mLocalSockfd(localSockfd), mLocalPort(localPort), mDestAddr(destIp, destPort),
        mIsAlive(false), mSessionId(0)
    {   }

private:
    int mLocalSockfd;
    uint16_t mLocalPort;
    Ipv4Address mDestAddr;
    bool mIsAlive;
    uint16_t mSessionId;
};

#endif