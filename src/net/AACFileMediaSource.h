#ifndef _AACFILE_MEDIA_SOURCE_H_
#define _AACFILE_MEDIA_SOURCE_H_

#include "MediaSource.h"

#include <string>

class AACFileMediaSource : public MediaSource {
public:
    static AACFileMediaSource *createNew(UsageEnvironment *usageEnvironment,
                                  std::string file);
    AACFileMediaSource(UsageEnvironment *usageEnvironment,
                       const std::string &file);
    ~AACFileMediaSource();

protected:
    virtual void readFrame();

private:
    struct AdtsHeader {
        unsigned int syncword; // 12
        unsigned int id; // 1
        unsigned int layer; // 2
        unsigned int protectionAbsent; // 1
        unsigned int profile; // 2
        unsigned int samplingFreqIndex; // 4
        unsigned int privateBit; // 1
        unsigned int channelCfg; // 3
        unsigned int originalCopy; // 1
        unsigned int home; // 1

        unsigned int copyRightIdentificationBit; // 1
        unsigned int copyRightIdentificationStart; // 1
        unsigned int aacFrameLength; // 13
        unsigned int adtsBufferFullness; // 11

        unsigned int numberOfRawDataBlockInFrame; // 1
    };

    bool parseAdtsHeader(uint8_t *in, struct AdtsHeader *res);
    int getFrameFromAACFile(int fd, uint8_t *frame, int size);

private:
    int mFd;
    std::string mFile;
    AdtsHeader mAdtsHeader;
};  

#endif