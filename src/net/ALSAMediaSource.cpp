#include "ALSAMediaSource.h"
#include "../base/New.h"
#include "../base/Logging.h"

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/avutil.h"
#include "libswresample/swresample.h"
#include "libavutil/opt.h"
}

ALSAMediaSource *ALSAMediaSource::createNew(UsageEnvironment *usageEnvironment, std::string device)
{
    return New<ALSAMediaSource>::allocate(usageEnvironment, device);
}

ALSAMediaSource::ALSAMediaSource(UsageEnvironment *usageEnvironment, const std::string &device) :
    MediaSource(usageEnvironment),
    mDevice(device),
    mChannels(2),
    mSampleRate(48000),
    mFormat(SND_PCM_FORMAT_S16),
    mFrames(1024)
{
    if(!alsaInit() || !swrInit()) {
        LOG_ERROR("Cannot init %s device\n", mDevice);
        _exit(-1);
    }

    /*--------------------------------------------*/
    if(avformat_alloc_output_context2(&fmtCtx, nullptr, nullptr, "output.aac") < 0)
        LOG_ERROR("avformat_alloc_output_context2\n");
    
    st = avformat_new_stream(fmtCtx, nullptr);
    if(!st)
        LOG_ERROR("avformat_new_stream\n");

    if(avcodec_parameters_from_context(st->codecpar, mCodecCtx) < 0)
        LOG_ERROR("avcodec_parameters_from_context\n");

    if(avio_open(&fmtCtx->pb, "output.aac", AVIO_FLAG_READ_WRITE) < 0)
        LOG_ERROR("avio_open\n");

    if(avformat_write_header(fmtCtx, nullptr) < 0)
        LOG_ERROR("avformat_write_header\n");
    /*--------------------------------------------*/

    setFps(mSampleRate/mFrames);
    printf("mSampleRate = %d --- mFrames = %ld\n", mSampleRate, mFrames);

    for(int i = 0; i < FRAME_DEFAULT_NUM; i++)
        usageEnvironment->threadPool()->addTask(mTask);

    
}

ALSAMediaSource::~ALSAMediaSource()
{
    alsaFree();
    swrFree();
}

void ALSAMediaSource::readFrame()
{
    MutexLockGuard lockInput(mMutexInput);

    if(mMyAVFrameInputQueue.empty())
        return ;
    
    MyAVFrame *frame = mMyAVFrameInputQueue.front();
    int ret = 0;
    static uint64_t pts = 0;
    bool success = false;

    while(!success) {
        ret = snd_pcm_readi(mPcmHandle, mPcmBuffer[0], mFrames);
        if(ret < 0) {
            LOG_ERROR("Cannot read alsa data: %s\n", snd_strerror(ret));
            continue;
        }
        else if(ret != mFrames) {
            LOG_WARNING("The data is short: %s\n", snd_strerror(ret));
            continue;
        }

        ret = swr_convert(mSwrCtx, mSwrFrame->data, mSwrFrame->nb_samples,
                          (const uint8_t **)mPcmBuffer, mFrames);
        if(ret < 0) {
            LOG_WARNING("Cannot convert pcm to aac data\n");
            continue;
        }
        mSwrFrame->pts = pts++;

        ret = avcodec_send_frame(mCodecCtx, mSwrFrame);
        if(ret < 0) {
            LOG_WARNING("Cannot send aac frame for encoder context\n");
            continue;
        }

        while (ret >= 0)
        {
            ret = avcodec_receive_packet(mCodecCtx, mSwrPacket);
            if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                // LOG_WARNING("The alsa context queue is empty pts = %ld\n", pts);
                continue;
            }
            else if(ret < 0) {
                LOG_WARNING("The context queue is error\n");
                break;
            }

            success = true;
            break;
        }
    }

    AVPacket *tmpPacket = av_packet_alloc();
    memcpy(tmpPacket, mSwrPacket, sizeof(AVPacket));
    if(av_packet_ref(tmpPacket, mSwrPacket) < 0)
        LOG_ERROR("av_packet_ref");

    if(pts < 500) {
        tmpPacket->stream_index = 0;
        if(av_interleaved_write_frame(fmtCtx, tmpPacket) < 0)
            LOG_ERROR("av_interleaved_write_frame");
    }
    else if(pts == 500) {
        LOG_DEBUG("finish trailer\n");
        av_write_trailer(fmtCtx);
    }

    MutexLockGuard lockOutput(mMutexOutput);

    memcpy(frame->mBuffer, mSwrPacket->data, mSwrPacket->size);
    frame->mFrame = frame->mBuffer;
    frame->mFrameSize = mSwrPacket->size;
    
    mMyAVFrameInputQueue.pop();
    mMyAVFrameOutputQueue.push(frame);
}

bool ALSAMediaSource::alsaInit()
{
    int ret;

    ret = snd_pcm_open(&mPcmHandle, mDevice.c_str(), SND_PCM_STREAM_CAPTURE, 0);
    if(ret < 0) {
        LOG_ERROR("Cannot open %s device\n", mDevice);
        return false;
    }

    ret = snd_pcm_hw_params_malloc(&mPcmParams);
    if(ret < 0) {
        LOG_ERROR("Cannot malloc %s pcm params\n", mDevice);
        return false;
    }

    ret = snd_pcm_hw_params_any(mPcmHandle, mPcmParams);
    if(ret < 0) {
        LOG_ERROR("Cannot set hardware default %s params\n", mDevice);
        return false;
    }

    ret = snd_pcm_hw_params_set_access(mPcmHandle, mPcmParams, SND_PCM_ACCESS_RW_INTERLEAVED);
    if(ret < 0) {
        LOG_ERROR("Cannot hardware set access mode\n");
        return false;
    }

    ret = snd_pcm_hw_params_set_channels(mPcmHandle, mPcmParams, mChannels);
    if(ret < 0) {
        LOG_ERROR("Cannot set hardware channels\n");
        return false;
    }

    ret = snd_pcm_hw_params_set_format(mPcmHandle, mPcmParams, mFormat);
    if(ret < 0) {
        LOG_ERROR("Cannot set hardware format\n");
        return -1;
    }

    ret = snd_pcm_hw_params_set_period_size_near(mPcmHandle, mPcmParams, &mFrames, NULL);
    if(ret < 0) {
        LOG_ERROR("Cannot set hardware period size\n");
        return -1;
    }

    ret = snd_pcm_hw_params_set_rate_near(mPcmHandle, mPcmParams, &mSampleRate, NULL);
    if(ret < 0) {
        LOG_ERROR("Cannot set hardware rate\n");
        return -1;
    }

    ret = snd_pcm_hw_params(mPcmHandle, mPcmParams);
    if(ret < 0) {
        LOG_ERROR("Cannot set hardware all params\n");
        return -1;
    }

    mFrameSize = mFrames * 4;
    mPcmBuffer[0] = (uint8_t *)malloc(mFrameSize);

    ret = snd_pcm_prepare(mPcmHandle);
    if(ret < 0) {
        LOG_ERROR("The device cannot prepare\n");
        return false;
    }

    return true;
}

void ALSAMediaSource::alsaFree()
{
    snd_pcm_drain(mPcmHandle);
    snd_pcm_close(mPcmHandle);
    snd_pcm_hw_free(mPcmHandle);
    snd_pcm_hw_params_free(mPcmParams);

    free(mPcmBuffer[0]);
}

bool ALSAMediaSource::swrInit()
{
    int ret = 0;

    const AVCodec *encoder = avcodec_find_encoder(AV_CODEC_ID_AAC);
    if(!encoder) {
        LOG_WARNING("Cannot find aac encoder\n");
        return false;
    }

    mCodecCtx = avcodec_alloc_context3(encoder);
    if(!mCodecCtx) {
        LOG_WARNING("Cannot alloc encoder context\n");
        return false;
    }

    mCodecCtx->sample_rate = 48000;
    mCodecCtx->bit_rate = 12800;
    mCodecCtx->codec_id = AV_CODEC_ID_AAC;
    mCodecCtx->codec_type = AVMEDIA_TYPE_AUDIO;
    mCodecCtx->sample_fmt = AV_SAMPLE_FMT_FLTP;
    mCodecCtx->ch_layout = AV_CHANNEL_LAYOUT_STEREO;
    mCodecCtx->profile = FF_PROFILE_AAC_LOW;


    ret = avcodec_open2(mCodecCtx, encoder, nullptr);
    if(ret < 0) {
        LOG_ERROR("Cannot open encoder\n");
        return false;
    }

    mSwrFrame = av_frame_alloc();
    mSwrPacket = av_packet_alloc();
    if(!mSwrFrame || !mSwrPacket) {
        LOG_WARNING("Cannot alloc frame or packet\n");
        return false;
    }

    mSwrFrame->ch_layout = AV_CHANNEL_LAYOUT_STEREO;
    mSwrFrame->sample_rate = mCodecCtx->sample_rate;
    mSwrFrame->format = mCodecCtx->sample_fmt;
    mSwrFrame->nb_samples = mCodecCtx->frame_size;

    ret = av_frame_get_buffer(mSwrFrame, 1);
    if(ret < 0) {
        LOG_DEBUG("Cannot get buffer for frame\n");
        return false;
    }

    printf("mCodecCtx->sample_rate = %d\n", mCodecCtx->sample_rate);

    AVChannelLayout layout = AV_CHANNEL_LAYOUT_STEREO;
    ret = swr_alloc_set_opts2(&mSwrCtx, &mCodecCtx->ch_layout, mCodecCtx->sample_fmt, mCodecCtx->sample_rate,
                              &layout, AV_SAMPLE_FMT_S16, mSampleRate, 0, NULL);
    if(ret < 0 || !mSwrCtx || swr_init(mSwrCtx) < 0) {
        LOG_DEBUG("Cannot init swr context\n");
        return false;
    }

    return true;
}

void ALSAMediaSource::swrFree()
{
    avcodec_free_context(&mCodecCtx);
    av_frame_free(&mSwrFrame);
    av_packet_free(&mSwrPacket);
    swr_free(&mSwrCtx);
}