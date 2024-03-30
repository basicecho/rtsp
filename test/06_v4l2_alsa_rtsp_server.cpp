#include "../src/base/Logging.h"
#include "../src/base/ThreadPool.h"
#include "../src/net/Event.h"
#include "../src/net/EventScheduler.h"
#include "../src/net/UsageEnvironment.h"
#include "../src/net/MediaSession.h"
#include "../src/net/RtspServer.h"
#include "../src/net/InetAddress.h"
#include "../src/net/H264RtpSink.h"
#include "../src/net/AACRtpSink.h"
#include "../src/net/V4L2MediaSource.h"
#include "../src/net/ALSAMediaSource.h"

#include <iostream>

int main(int argc, char **argv)
{
    // if(argc != 3)
    // {
    //     std::cout << "Usage: " << argv[0] << " <h264 file > <aac file>" << std::endl;
    //     return -1;
    // }

    Logger::setLogLevel(Logger::LogWarning);

    EventScheduler* scheduler = EventScheduler::createNew(EventScheduler::POLLER_EPOLL);
    ThreadPool* threadPool = ThreadPool::createNew(5);
    UsageEnvironment* usageEnvironment = UsageEnvironment::createNew(scheduler, threadPool);

    Ipv4Address addr("0.0.0.0", 8554);
    RtspServer* server = RtspServer::createNew(usageEnvironment, addr);
    MediaSession* session = MediaSession::createNew("live");    
    MediaSource* videoSource = V4L2MediaSource::createNew(usageEnvironment, "/dev/video0");
    RtpSink* videoRtpSink = H264RtpSink::createNew(usageEnvironment, videoSource);
    MediaSource *audioSource = ALSAMediaSource::createNew(usageEnvironment, "hw:0");
    RtpSink* audioRtpSink = AACRtpSink::createNew(usageEnvironment, audioSource);

    session->addRtpSink(MediaSession::TrackId0, videoRtpSink);
    session->addRtpSink(MediaSession::TrackId1, audioRtpSink);

    server->addMediaSession(session);
    server->start();

    std::cout << server->getUrl(session) << std::endl;

    usageEnvironment->scheduler()->loop();

    return 0;
}