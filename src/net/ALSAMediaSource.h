#ifndef _ALSA_MEDIA_SOURCE_H_
#define _ALSA_MEDIA_SOURCE_H_

#include "MediaSource.h"
extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/avutil.h"
#include "libswresample/swresample.h"
}

#include <alsa/asoundlib.h>
#include <stdint.h>
#include <string>
#include <queue>

class ALSAMediaSource : public MediaSource {
public:
    static ALSAMediaSource *createNew(UsageEnvironment *usageEnvironment, std::string device);
    ALSAMediaSource(UsageEnvironment *usageEnvironment, const std::string &device);
    virtual ~ALSAMediaSource();

protected:
    virtual void readFrame();

private:
    bool alsaInit();
    void alsaFree();
    bool swrInit();
    void swrFree();

private:
    std::string mDevice;
    snd_pcm_t *mPcmHandle;

    snd_pcm_hw_params_t *mPcmParams;
    unsigned int mChannels;
    snd_pcm_format_t mFormat;
    unsigned int mSampleRate;
    snd_pcm_uframes_t mFrames;
    int mFrameSize;
    uint8_t *mPcmBuffer[1];

    AVCodecContext *mCodecCtx;
    struct SwrContext *mSwrCtx;
    AVFrame *mSwrFrame;
    AVPacket *mSwrPacket;

    /*---------------------------*/
    AVFormatContext *fmtCtx;
    AVStream *st;
    /*---------------------------*/
};

#endif