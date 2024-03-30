#ifndef _RTSP_SERVER_H_
#define _RTSP_SERVER_H_ 

#include "../base/Mutex.h"
#include "Event.h"
#include "TcpServer.h"
#include "RtspConnection.h"
#include "MediaSession.h"
#include "UsageEnvironment.h"

#include <map>
#include <vector>
#include <string>

class RtspConnection;

class RtspServer : public TcpServer {
public:
    static RtspServer *createNew(UsageEnvironment *usageEnvironment,
                         Ipv4Address &addr);
    RtspServer(UsageEnvironment *usageEnvironment, const Ipv4Address &addr);
    virtual ~RtspServer();

    UsageEnvironment *usageEnvironment() { return mUsageEnvironment; }
    bool addMediaSession(MediaSession *mediaSession);
    MediaSession *loopupMediaSession(std::string name);
    std::string getUrl(MediaSession *session);

protected:
    virtual void handleNewConnection(int connfd);
    void handleDisconnection(int sockfd);
    void handleDisconnectionList();

    static void disconnectionCallback(void *arg, int sockfd);
    static void triggerCallback(void *arg);

private:
    std::map<std::string, MediaSession *> mMediaSessions;
    std::map<int, RtspConnection *> mConnections;
    std::vector<int> mDisconnectionList;
    TriggerEvent *mTriggerEvent;
    Mutex *mMutex;
};

#endif