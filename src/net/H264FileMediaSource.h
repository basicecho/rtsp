#ifndef _H264FILE_MEDIA_SOURCE_H_
#define _H264FILE_MEDIA_SOURCE_H_

#include "MediaSource.h"
#include "UsageEnvironment.h"

#include <string>

class H264FileMediaSource : public MediaSource {
public:
    static H264FileMediaSource *createNew(UsageEnvironment *usageEnvironment,
                                   std::string file);
    H264FileMediaSource(UsageEnvironment *usageEnvironment,
                        const std::string &file);
    ~H264FileMediaSource();

protected:
    virtual void readFrame();

private:
    int getFrameFromH264File(int fd, uint8_t *frame, int size);

private:
    int mFd;
    std::string mFile;
};

#endif