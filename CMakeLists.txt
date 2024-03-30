cmake_minimum_required(VERSION 3.0)
project(RtspServer)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Debug)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/test)

set(SOURCE_LISTS
    src/base/Allocator.cpp
    src/base/Allocator.h
    src/base/AsyncLogging.cpp
    src/base/AsyncLogging.h
    src/base/Condition.cpp
    src/base/Condition.h
    src/base/Construct.h
    src/base/Logging.cpp
    src/base/Logging.h
    src/base/Mutex.cpp
    src/base/Mutex.h
    src/base/New.h
    src/base/Sem.cpp
    src/base/Sem.h
    src/base/Thread.cpp
    src/base/Thread.h
    src/base/ThreadPool.cpp
    src/base/ThreadPool.h
    src/net/AACFileMediaSource.cpp
    src/net/AACFileMediaSource.h
    src/net/AACRtpSink.cpp
    src/net/AACRtpSink.h
    src/net/Acceptor.cpp
    src/net/Acceptor.h
    src/net/Buffer.h
    src/net/Event.h
    src/net/EventScheduler.cpp
    src/net/EventScheduler.h
    src/net/H264FileMediaSource.cpp
    src/net/H264FileMediaSource.h
    src/net/H264RtpSink.cpp
    src/net/H264RtpSink.h
    src/net/InetAddress.cpp
    src/net/InetAddress.h
    src/net/MediaSession.cpp
    src/net/MediaSession.h
    src/net/MediaSource.cpp
    src/net/MediaSource.h
    src/net/Rtp.h
    src/net/RtpInstance.h
    src/net/RtpSink.cpp
    src/net/RtpSink.h
    src/net/RtspConnection.cpp
    src/net/RtspConnection.h
    src/net/RtspServer.cpp
    src/net/RtspServer.h
    src/net/SocketsOps.cpp
    src/net/SocketsOps.h
    src/net/TcpConnection.cpp
    src/net/TcpConnection.h
    src/net/TcpServer.h
    src/net/TcpSocket.h
    src/net/Timer.cpp
    src/net/Timer.h
    src/net/UsageEnvironment.h
    src/net/poller/EPollPoller.cpp
    src/net/poller/EPollPoller.h
    src/net/poller/Poller.cpp
    src/net/poller/Poller.h
    src/net/poller/PollPoller.cpp
    src/net/poller/PollPoller.h
    src/net/poller/SelectPoller.cpp
    src/net/poller/SelectPoller.h
    src/net/ALSAMediaSource.h
    src/net/ALSAMediaSource.cpp
    src/net/V4L2MediaSource.h
    src/net/V4L2MediaSource.cpp
)

include_directories(src/base src/net src/net/poller src/net/extend)

add_executable(1_h264 test/01_h264_rtsp_server.cpp ${SOURCE_LISTS})
add_executable(2_aac  test/02_aac_rtsp_server.cpp ${SOURCE_LISTS})
add_executable(3_h264_aac test/03_h264_aac_rtsp_server.cpp ${SOURCE_LISTS})
add_executable(4_v4l2 test/04_v4l2_rtsp_server.cpp ${SOURCE_LISTS})
add_executable(5_alsa test/05_alsa_rtsp_server.cpp ${SOURCE_LISTS})
add_executable(6_v4l2_alsa test/06_v4l2_alsa_rtsp_server.cpp ${SOURCE_LISTS})

target_link_libraries(1_h264 -lpthread -lavformat -lavcodec -lavutil -lavdevice -lswscale -lswresample -lasound)
target_link_libraries(2_aac -lpthread -lavformat -lavcodec -lavutil -lavdevice -lswscale -lswresample -lasound)
target_link_libraries(3_h264_aac -lpthread -lavformat -lavcodec -lavutil -lavdevice -lswscale -lswresample -lasound)
target_link_libraries(4_v4l2 -lpthread -lavformat -lavcodec -lavutil -lavdevice -lswscale -lswresample -lasound)
target_link_libraries(5_alsa -lpthread -lavformat -lavcodec -lavutil -lavdevice -lswscale -lswresample -lasound)
target_link_libraries(6_v4l2_alsa -lpthread -lavformat -lavcodec -lavutil -lavdevice -lswscale -lswresample -lasound)