#ifndef _V4L2_MEDIA_SOURCE_H_
#define _V4L2_MEDIA_SOURCE_H_

#include "MediaSource.h"
extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
}
#include <linux/videodev2.h>

#include <string>
#include <queue>

class V4L2MediaSource : public MediaSource {
public:
    static V4L2MediaSource *createNew(UsageEnvironment *usageEnvironment, std::string device);
    V4L2MediaSource(UsageEnvironment *usageEnvironment, const std::string &device);
    virtual ~V4L2MediaSource();

protected:
    virtual void readFrame();

private:
    bool yuvInit();
    void yuvFree();
    bool swsInit();
    void swsFree();

private:
    std::string mDevice;
    int mFd;
    int mWidth;
    int mHeight;
    char *mYUVBuffer[FRAME_DEFAULT_NUM];
    int mYUVSize[FRAME_DEFAULT_NUM];

    AVFormatContext *mFmtCtx;
    AVCodecContext *mCodecCtx;
    AVFrame *mSwsFrame;
    AVPacket *mSwsPacket;
    struct SwsContext *mSwsCtx;

};

#endif