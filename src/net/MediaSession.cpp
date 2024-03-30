#include "MediaSession.h"
#include "../base/New.h"
#include "../base/Logging.h"

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <algorithm>

MediaSession *MediaSession::createNew(std::string sessionName)
{
    return New<MediaSession>::allocate(sessionName);
}

MediaSession::MediaSession(const std::string &sessionName) :
    mSessionName(sessionName)
{
    mTracks[0].mTrackId = TrackId0;
    mTracks[1].mTrackId = TrackId1;
    mTracks[0].mIsAlive = false;
    mTracks[1].mIsAlive = false;
}

MediaSession::~MediaSession()
{

}

std::string MediaSession::generateSDPDescription()
{
    if(!mSdp.empty())
        return mSdp;
    
    std::string ip = sockets::getLocalIp();
    char buf[2048] = { 0 };

    sprintf(buf,
            "v=0\r\n"
            "o=- 9%ld 1 IN IP4 %s\r\n"
            "t=0 0\r\n"
            "a=control:*\r\n"
            "a=type:broadcast\r\n",
            (long)time(nullptr),
            ip.c_str()
            );

    for(int i = 0; i < MEDIA_MAX_TRACK_NUM; i++) {
        uint16_t port = 0;

        if(mTracks[i].mIsAlive == false)
            continue;
        
        sprintf(buf + strlen(buf), "%s\r\n",
                mTracks[i].mRtpSink->getMediaDescription(port).c_str());
        sprintf(buf + strlen(buf), "c=IN IP4 0.0.0.0\r\n");
        sprintf(buf + strlen(buf), "%s\r\n",
                mTracks[i].mRtpSink->getAttribute().c_str());
        sprintf(buf + strlen(buf), "a=control:track%d\r\n",
                mTracks[i].mTrackId);
    }

    mSdp = std::string(buf);
    return mSdp;
}

bool MediaSession::addRtpSink(MediaSession::TrackId trackId, RtpSink *rtpSink)
{
    Track *track = getTrack(trackId);
    if(!track)
        return false;
    
    track->mRtpSink = rtpSink;
    track->mIsAlive = true;

    rtpSink->setSendPacketCallback(sendPacketCallback, this, track);

    return true;
}

bool MediaSession::addRtpInstance(MediaSession::TrackId trackId, RtpInstance *rtpInstance)
{
    Track *track = getTrack(trackId);
    if(!track || !track->mIsAlive)
        return false;
    
    track->mRtpInstances.push_back(rtpInstance);

    return true;
}

bool MediaSession::removeRtpInstance(RtpInstance *rtpInstance)
{
    for(int i = 0; i < MEDIA_MAX_TRACK_NUM; i++) {
        if(mTracks[i].mIsAlive == false)
            continue;
        
        std::list<RtpInstance *>::iterator it = std::find(mTracks[i].mRtpInstances.begin(),
                                                          mTracks[i].mRtpInstances.end(),
                                                          rtpInstance);

        if(it == mTracks[i].mRtpInstances.end())
            continue;
        
        mTracks->mRtpInstances.erase(it);
        return true;
    }

    return false;
}

MediaSession::Track *MediaSession::getTrack(TrackId trackId)
{
    for(int i = 0; i < MEDIA_MAX_TRACK_NUM; i++) {
        if(mTracks[i].mTrackId == trackId)
            return &mTracks[i];
    }
    
    return nullptr;
}

void MediaSession::sendPacketCallback(void *arg1, void *arg2, RtpPacket *rtpPacket)
{
    MediaSession *mediaSession = (MediaSession *)arg1;
    MediaSession::Track *track = (MediaSession::Track *)arg2;

    mediaSession->sendPacket(track, rtpPacket);
}

void MediaSession::sendPacket(MediaSession::Track *track, RtpPacket *rtpPacket)
{
    for(std::list<RtpInstance *>::iterator it = track->mRtpInstances.begin();
        it != track->mRtpInstances.end(); it++) {
        if((*it)->alive())
            (*it)->send(rtpPacket);
    }
}