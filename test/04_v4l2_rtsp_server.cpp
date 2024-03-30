#include "../src/base/Logging.h"
#include "../src/base/ThreadPool.h"
#include "../src/net/UsageEnvironment.h"
#include "../src/net/EventScheduler.h"
#include "../src/net/Event.h"
#include "../src/net/RtspServer.h"
#include "../src/net/MediaSession.h"
#include "../src/net/InetAddress.h" 
#include "../src/net/V4L2MediaSource.h"
#include "../src/net/H264RtpSink.h"

#include <iostream>
 
int main(int argc, char **argv)
{
    Logger::setLogLevel(Logger::LogDebug);

    EventScheduler *scheduler = EventScheduler::createNew(EventScheduler::POLLER_SELECT);
    ThreadPool *threadPool = ThreadPool::createNew(2);
    UsageEnvironment *usageEnvironment = UsageEnvironment::createNew(scheduler, threadPool);

    Ipv4Address addr("0.0.0.0", 8554);
    RtspServer *server = RtspServer::createNew(usageEnvironment, addr);
    MediaSession *session = MediaSession::createNew("live");
    MediaSource *mediaSource = V4L2MediaSource::createNew(usageEnvironment, "/dev/video0");
    RtpSink *rtpSink = H264RtpSink::createNew(usageEnvironment, mediaSource);

    session->addRtpSink(MediaSession::TrackId0, rtpSink);

    server->addMediaSession(session);
    server->start();

    std::cout << server->getUrl(session) << std::endl;

    usageEnvironment->scheduler()->loop();
    return 0;
}