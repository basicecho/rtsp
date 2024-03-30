#include "RtspServer.h"
#include "../base/New.h"
#include "../base/Logging.h"

#include <stdio.h>
#include <assert.h>
#include <algorithm>

RtspServer *RtspServer::createNew(UsageEnvironment *usageEnvironment,
                                  Ipv4Address &addr)
{
    return New<RtspServer>::allocate(usageEnvironment, addr);
}

RtspServer::RtspServer(UsageEnvironment *usageEnvironment, const Ipv4Address &addr) :
    TcpServer(usageEnvironment, addr)
{
    mTriggerEvent = TriggerEvent::createNew(this);
    mTriggerEvent->setTriggerCallback(triggerCallback);

    mMutex = Mutex::createNew();
}

RtspServer::~RtspServer()
{
    Delete::release(mTriggerEvent);
    Delete::release(mMutex);
}

void RtspServer::handleNewConnection(int connfd)
{
    RtspConnection *conn = RtspConnection::createNew(this, connfd);
    conn->setDisconnectionCallback(disconnectionCallback, this);
    mConnections.insert(std::make_pair(connfd, conn));
}

void RtspServer::disconnectionCallback(void *arg, int sockfd)
{
    RtspServer *rtspServer = (RtspServer *)arg;
    rtspServer->handleDisconnection(sockfd);
}

void RtspServer::handleDisconnection(int sockfd)
{
    MutexLockGuard lock(mMutex);
    mDisconnectionList.push_back(sockfd);
    mUsageEnvironment->scheduler()->addTriggerEvent(mTriggerEvent);
}

bool RtspServer::addMediaSession(MediaSession *mediaSession)
{
    if(mMediaSessions.find(mediaSession->name()) != mMediaSessions.end())
        return false;
    
    mMediaSessions.insert(std::make_pair(mediaSession->name(), mediaSession));

    return true;
}

MediaSession *RtspServer::loopupMediaSession(std::string name)
{
    std::map<std::string, MediaSession *>::iterator it = mMediaSessions.find(name);
    if(it == mMediaSessions.end())
        return nullptr;
    
    return it->second;
}

std::string RtspServer::getUrl(MediaSession *session)
{
    char url[200];

    snprintf(url, sizeof(url), "rtsp://%s:%d/%s", sockets::getLocalIp().c_str(),
             mAddr.getPort(), session->name().c_str());
            
    return std::string(url);
}

void RtspServer::triggerCallback(void *arg)
{
    RtspServer *rtspServer = (RtspServer *)arg;
    rtspServer->handleDisconnectionList();
}

void RtspServer::handleDisconnectionList()
{
    MutexLockGuard lock(mMutex);

    for(std::vector<int>::iterator it = mDisconnectionList.begin();
        it != mDisconnectionList.end(); it++) {
        int sockfd = *it;
        std::map<int, RtspConnection *>::iterator _it = mConnections.find(sockfd);
        assert(_it != mConnections.end());

        Delete::release(_it->second);
        mConnections.erase(sockfd);
    }

    mDisconnectionList.clear();
}