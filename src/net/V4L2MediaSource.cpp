#include "V4L2MediaSource.h"
#include "../base/New.h"
#include "../base/Logging.h"

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/avutil.h"
#include "libswscale/swscale.h"
}

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

V4L2MediaSource *V4L2MediaSource::createNew(UsageEnvironment *usageEnvironment, std::string device)
{
    return New<V4L2MediaSource>::allocate(usageEnvironment, device);
}

V4L2MediaSource::V4L2MediaSource(UsageEnvironment *usageEnvironment, const std::string &device) :
    MediaSource(usageEnvironment),
    mDevice(device),
    mWidth(640),
    mHeight(480)
{
    setFps(25);

    if(!yuvInit() || !swsInit()) {
        LOG_ERROR("Cannot init %s device\n", mDevice);
        _exit(-1);
    }

    for(int i = 0; i < FRAME_DEFAULT_NUM; i++)
        mUsageEnvironment->threadPool()->addTask(mTask);
}

V4L2MediaSource::~V4L2MediaSource()
{
    yuvFree();
    swsFree();
}

void V4L2MediaSource::readFrame()
{
    MutexLockGuard lockInput(mMutexInput);

    if(mMyAVFrameInputQueue.empty())
        return ;
    
    MyAVFrame *frame = mMyAVFrameInputQueue.front();

    int ret = 0;
    static uint64_t pts = 0;
    struct v4l2_buffer buf;
    
    char *tmpBuffer[1];
    int tmpLength[1];
    tmpLength[0] = mSwsFrame->linesize[0] + mSwsFrame->linesize[1] + mSwsFrame->linesize[2];
    bool success = false;

    while(!success) {
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        ret = ioctl(mFd, VIDIOC_DQBUF, &buf);
        if(ret < 0) {
            LOG_WARNING("Cannot get video frame from %s\n", mDevice);
            continue;
        }

        tmpBuffer[0] = mYUVBuffer[buf.index];
        
        ret = sws_scale(mSwsCtx, (const uint8_t **)tmpBuffer, tmpLength, 0, mHeight,
                        (uint8_t **)mSwsFrame->data, mSwsFrame->linesize);
        if(ret < 0) {
            LOG_WARNING("Cannot convert video frame\n");
            goto error;
        }

        mSwsFrame->pts = pts++;

        // printf("pts = %d\n", pts);
        ret = avcodec_send_frame(mCodecCtx, mSwsFrame);
        if(ret < 0) {
            LOG_WARNING("Cannot send frame from video encoder context\n");
            goto error;
        }

        while(ret >= 0) {
            ret = avcodec_receive_packet(mCodecCtx, mSwsPacket);
            if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                // LOG_WARNING("The context queue is empty  pts = %d\n", pts);
                break;
            }
            else if(ret < 0) {
                LOG_WARNING("The context queue is error\n");
                break;
            }

            success = true;
            break;
        }

        error:
        ret = ioctl(mFd, VIDIOC_QBUF, &buf);
        if(ret < 0) {
            LOG_WARNING("Cannot pop video frame\n");
        }
    }
    MutexLockGuard lockOutput(mMutexOutput);

    memcpy(frame->mBuffer, mSwsPacket->data, mSwsPacket->size);
    frame->mFrame = frame->mBuffer;
    frame->mFrameSize = mSwsPacket->size;
    
    mMyAVFrameInputQueue.pop();
    mMyAVFrameOutputQueue.push(frame);
}

bool V4L2MediaSource::yuvInit()
{
    int ret = 0;
    struct v4l2_format format;
    struct v4l2_requestbuffers reqs;

    mFd = open(mDevice.c_str(), O_RDWR);
    if(mFd < 0) {
        LOG_ERROR("Cannot open %s video\n", mDevice);
        return false;
    }

    memset(&format, 0, sizeof(format));
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.width = mWidth;
    format.fmt.pix.height = mHeight;
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    ret = ioctl(mFd, VIDIOC_S_FMT, &format);
    if(ret < 0) {
        LOG_ERROR("Failed to set video format\n");
        return false;
    }

    reqs.count = FRAME_DEFAULT_NUM;
    reqs.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqs.memory = V4L2_MEMORY_MMAP;
    ret = ioctl(mFd, VIDIOC_REQBUFS, &reqs);
    if(ret < 0) {
        printf("Failed to request buffers\n");
        return false;
    }

    struct v4l2_buffer mapBuffer;
    for(int i = 0; i < FRAME_DEFAULT_NUM; i++) {
        mapBuffer.index = i;
        mapBuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        mapBuffer.memory = V4L2_MEMORY_MMAP;
        ret = ioctl(mFd, VIDIOC_QUERYBUF, &mapBuffer);
        if(ret < 0) {
            printf("Cannot get %d buffer from video stream\n", i);
            return false;
        }

        mYUVBuffer[i] = (char *)mmap(NULL, mapBuffer.length, PROT_WRITE | PROT_READ, 
                                     MAP_SHARED, mFd, mapBuffer.m.offset);
        mYUVSize[i] = mapBuffer.length;

        ret = ioctl(mFd, VIDIOC_QBUF, &mapBuffer);
        if(ret < 0) {
            printf("Failed to push %d raw frame to video queue\n", i);
            return false;
        }
    }

    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ret = ioctl(mFd, VIDIOC_STREAMON, &type);
    if(ret < 0) {
        LOG_ERROR("Cannot on video stream\n");
        return false;
    }

    return true;
}

void V4L2MediaSource::yuvFree()
{
    int ret = 0;

    for(int i = 0; i < FRAME_DEFAULT_NUM; i++) {
        munmap(mYUVBuffer[i], mYUVSize[i]);
    }
    close(mFd);

    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ret = ioctl(mFd, VIDIOC_STREAMOFF, &type);
    if(ret < 0) {
        LOG_WARNING("Cannot close video stream\n");
    }
}

bool V4L2MediaSource::swsInit()
{
    int ret = 0;

    const AVCodec *encoder = avcodec_find_encoder(AV_CODEC_ID_H264);

    mCodecCtx = avcodec_alloc_context3(encoder);
    if(!mCodecCtx) {
        LOG_ERROR("Cannot alloc video device\n");
        return false;
    }

    mCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
    mCodecCtx->codec_id = AV_CODEC_ID_H264;
    mCodecCtx->width = mWidth;
    mCodecCtx->height = mHeight;
    mCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    mCodecCtx->gop_size = 10;
    mCodecCtx->max_b_frames = 1;
    mCodecCtx->time_base = { 1, 25 };
    mCodecCtx->framerate = { 25, 1 };

    ret = avcodec_open2(mCodecCtx, encoder, NULL);
    if(ret < 0) {
        LOG_ERROR("Cannot open video encoder context\n");
        return false;
    }

    mSwsFrame = av_frame_alloc();
    mSwsPacket = av_packet_alloc();

    mSwsFrame->width = mWidth;
    mSwsFrame->height = mHeight;
    mSwsFrame->format = AV_PIX_FMT_YUV420P;
    ret = av_frame_get_buffer(mSwsFrame, 1);
    if(ret < 0) {
        LOG_ERROR("Cannot get h264 video buffer\n");
        return false;
    }

    mSwsCtx = sws_getContext(mWidth, mHeight, AV_PIX_FMT_YUYV422, 
                             mWidth, mHeight, AV_PIX_FMT_YUV420P,
                             SWS_BICUBIC, NULL, NULL, NULL);
    if(!mSwsCtx) {
        LOG_ERROR("Cannot get sws context\n");
        return false;
    }
    
    return true;
}
void V4L2MediaSource::swsFree()
{
     sws_freeContext(mSwsCtx);
     avcodec_free_context(&mCodecCtx);
     av_frame_free(&mSwsFrame);
     av_packet_free(&mSwsPacket);
}