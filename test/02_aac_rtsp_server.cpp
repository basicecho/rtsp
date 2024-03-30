#include "../src/base/Logging.h"
#include "../src/base/ThreadPool.h"
#include "../src/net/UsageEnvironment.h"
#include "../src/net/EventScheduler.h"
#include "../src/net/RtspServer.h"
#include "../src/net/InetAddress.h"
#include "../src/net/MediaSession.h"
#include "../src/net/AACFileMediaSource.h"
#include "../src/net/AACRtpSink.h"
#include "../src/net/Event.h"

#include <iostream>

int main(int argc, char **argv)
{
    if(argc != 2) {
        std::cout << "Usage: " << argv[0] << "<aac file>" << std::endl;
        return -1;
    }

    Logger::setLogLevel(Logger::LogWarning);

    EventScheduler *scheduler = EventScheduler::createNew(EventScheduler::POLLER_SELECT);
    ThreadPool *threadPool = ThreadPool::createNew(2);
    UsageEnvironment *usageEnvironment = UsageEnvironment::createNew(scheduler, threadPool);

    Ipv4Address addr("0.0.0.0", 8554);
    RtspServer *server = RtspServer::createNew(usageEnvironment, addr);
    MediaSession *session = MediaSession::createNew("live");
    MediaSource *MediaSource = AACFileMediaSource::createNew(usageEnvironment, argv[1]);
    RtpSink *rtpSink = AACRtpSink::createNew(usageEnvironment, MediaSource);

    session->addRtpSink(MediaSession::TrackId0, rtpSink);

    std::cout << server->getUrl(session) << std::endl;

    server->addMediaSession(session);
    server->start();

    usageEnvironment->scheduler()->loop();

    return 0;
}