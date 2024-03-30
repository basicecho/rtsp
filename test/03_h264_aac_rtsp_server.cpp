#include "../src/base/Logging.h"
#include "../src/base/ThreadPool.h"
#include "../src/net/Event.h"
#include "../src/net/EventScheduler.h"
#include "../src/net/UsageEnvironment.h"
#include "../src/net/MediaSession.h"
#include "../src/net/RtspServer.h"
#include "../src/net/InetAddress.h"
#include "../src/net/H264FileMediaSource.h"
#include "../src/net/H264RtpSink.h"
#include "../src/net/AACFileMediaSource.h"
#include "../src/net/AACRtpSink.h"

#include <iostream>

int main(int argc, char **argv)
{
    if(argc != 3)
    {
        std::cout << "Usage: " << argv[0] << " <h264 file > <aac file>" << std::endl;
        return -1;
    }

    Logger::setLogLevel(Logger::LogWarning);

    EventScheduler* scheduler = EventScheduler::createNew(EventScheduler::POLLER_SELECT);
    ThreadPool* threadPool = ThreadPool::createNew(2);
    UsageEnvironment* usageEnvironment = UsageEnvironment::createNew(scheduler, threadPool);

    Ipv4Address addr("0.0.0.0", 8554);
    RtspServer* server = RtspServer::createNew(usageEnvironment, addr);
    MediaSession* session = MediaSession::createNew("live");    
    MediaSource* videoSource = H264FileMediaSource::createNew(usageEnvironment, argv[1]);
    RtpSink* videoRtpSink = H264RtpSink::createNew(usageEnvironment, videoSource);
    MediaSource* audioSource = AACFileMediaSource::createNew(usageEnvironment, argv[2]);
    RtpSink* audioRtpSink = AACRtpSink::createNew(usageEnvironment, audioSource);

    session->addRtpSink(MediaSession::TrackId0, videoRtpSink);
    session->addRtpSink(MediaSession::TrackId1, audioRtpSink);

    server->addMediaSession(session);
    server->start();

    std::cout << server->getUrl(session) << std::endl;

    usageEnvironment->scheduler()->loop();

    return 0;
}