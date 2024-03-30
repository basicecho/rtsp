#ifndef _BUFFER_H_
#define _BUFFER_H_

#include "SocketsOps.h"

#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <assert.h>
#include <algorithm>

class Buffer {
public:
    static const int initialSize = 1024;
    static constexpr const char *kCRLF = "\r\n";

    explicit Buffer() : 
        mBufferSize(initialSize),
        mReadIndex(0),
        mWriteIndex(0)
    {
        mBuffer = (char *)malloc(mBufferSize);
    }

    ~Buffer()
    {
        free(mBuffer);
    }

    char *begin() { return mBuffer; }
    const char *begin() const { return mBuffer; }
    int readableBytes() { return mWriteIndex - mReadIndex; }
    int writableBytes() { return mBufferSize - mWriteIndex; }
    int prependableBytes() { return mReadIndex; }
    char *peek() { return begin() + mReadIndex; }
    const char *peek() const { return begin() + mReadIndex; }
    char *beginWrite() { return begin() + mWriteIndex; }
    const char *beginWrite() const { return begin() + mWriteIndex; }

    const char *findCRLF()
    {
        const char *crlf = std::search(peek(), beginWrite(), kCRLF, kCRLF  + 2);
        return crlf == beginWrite() ? nullptr : crlf;
    }

    const char *findCRLF(const char *start) const
    {
        assert(peek() <= start);
        assert(start <= beginWrite());
        const char *crlf = std::search(start, beginWrite(), kCRLF, kCRLF + 2);
        return crlf == beginWrite() ? nullptr : crlf;
    }

    const char *findLastCrlf()
    {
        const char *crlf = std::find_end(peek(), beginWrite(), kCRLF, kCRLF + 2);
        return crlf == beginWrite() ? nullptr : crlf;
    }

    void retrieveAll()
    {
        mReadIndex = 0;
        mWriteIndex = 0;
    }

    void retrieve(int len)
    {
        assert(len <= readableBytes());
        if(len < readableBytes())
            mReadIndex += len;
        else
            retrieveAll();
    }

    void retrieveUtil(const char *end)
    {
        assert(peek() <= end);
        assert(end <= beginWrite());
        retrieve(end - peek());
    }

    void hasWritten(int len)
    {
        int a = len;
        int b = readableBytes();
        assert(len <= writableBytes());
        mWriteIndex += len;
    }

    void unwrite(int len)
    {
        assert(len <= readableBytes());
        mWriteIndex -= len;
    }

    void makeSpace(int len)
    {
        if(writableBytes() + prependableBytes() < len)
        {
            mBufferSize = mWriteIndex + len;
            mBuffer = (char *)realloc(mBuffer, mBufferSize);
        }
        else {
            int readable = readableBytes();
            std::copy(begin() + mReadIndex, begin() + mWriteIndex, begin());
            mReadIndex = 0;
            mWriteIndex = mReadIndex + readable;
            assert(readable == readableBytes());
        }
    }

    void ensureWritableBytes(int len)
    {
        if(writableBytes() < len)
            makeSpace(len);
        assert(writableBytes() >= len);
    }

    void append(const char* data, int len)
    {
        ensureWritableBytes(len);
        std::copy(data, data+len, beginWrite());
        hasWritten(len); //重新调节写位置
    }

    void append(const void* data, int len)
    {
        append((const char*)(data), len);
    }

    int read(int fd)
    {
        char extrabuf[65536];
        struct iovec vec[2];
        const int writable = writableBytes();
        
        vec[0].iov_base = begin() + mWriteIndex;
        vec[0].iov_len = writable;
        vec[1].iov_base = extrabuf;
        vec[1].iov_len = sizeof(extrabuf);

        const int iovcnt = (writable < sizeof(extrabuf)) ? 2 : 1;
        const int n = sockets::readv(fd, vec, iovcnt);

        if(n < 0)
            return -1;
        else if(n <= writable)
            mWriteIndex += n;
        else {
            mWriteIndex = mBufferSize;
            append(extrabuf, n - writable);
        }

        return n;
    }

    int write(int fd)
    {
        return sockets::write(fd, peek(), readableBytes());
    }

private:
    char *mBuffer;
    int mBufferSize;
    int mReadIndex;
    int mWriteIndex;
};

#endif