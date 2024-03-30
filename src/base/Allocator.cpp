#include "Allocator.h"

#include <stdlib.h>
#include <iostream>

Allocator* Allocator::mAllocator = NULL;

void* Allocator::allocate(uint32_t size)
{
    return getInstance()->alloc(size);
}

void Allocator::deallocate(void* p, uint32_t size)
{
    getInstance()->dealloc(p, size);
}

Allocator* Allocator::getInstance()
{
    if(!mAllocator)
        mAllocator = new Allocator();

    return mAllocator;
}

void* Allocator::alloc(uint32_t size)
{
    Obj* result;
    uint32_t index;

    MutexLockGuard mutexLockGuard(mMutex);

    /* 如果分配内存大于 MAX_BYTES，那么就直接通过 malloc 分配 */
    if(size > MAX_BYTES)
        return malloc(size);
    
    index = freelistIndex(size);
    result = mFreeList[index];

    if(!result)
    {
        void* r = refill(roundup(size));
        return r;
    }

    mFreeList[index] = result->next;

    return result;
}

void Allocator::dealloc(void* p, uint32_t size)
{
    Obj* obj = (Obj*)p;
    uint32_t index;

    MutexLockGuard mutexLockGuard(mMutex);

    if(size > MAX_BYTES) {
        free(p);
        return ;
    }
        

    index = freelistIndex(size);

    obj->next = mFreeList[index];
    mFreeList[index] = obj;
}

void* Allocator::refill(uint32_t bytes)
{
    int nobjs = 20;
    char* chunk = chunkAlloc(bytes, nobjs);
    Obj* result;
    Obj* currentObj;
    Obj* nextObj;
    int i;
    uint32_t index;

    if(1 == nobjs)
        return chunk;

    result = (Obj*)chunk;
    index = freelistIndex(bytes);
    mFreeList[index] = nextObj = (Obj*)(chunk + bytes);

    for(i = 1; ; ++i)
    {
        currentObj = nextObj;
        nextObj = (Obj*)((char*)nextObj + bytes);
    
        if(nobjs-1 == i)
        {
            currentObj->next = 0;
            break;
        }
        else
        {
            currentObj->next = nextObj;
        }
    }

    return result;
}

char* Allocator::chunkAlloc(uint32_t size, int& nobjs)
{
    char* result;
    uint32_t totalBytes = size * nobjs;
    uint32_t bytesLeft = mEndFree - mStartFree;

    if(bytesLeft > totalBytes)
    {
        result = mStartFree;
        mStartFree += totalBytes;
        return result;
    }
    else if(bytesLeft > size)
    {
        nobjs = bytesLeft / size;
        totalBytes = size * nobjs;
        result = mStartFree;
        mStartFree += totalBytes;
        return result;
    }
    else
    {
        uint32_t bytesToGet = 2 * totalBytes + roundup(mHeapSize >> 4);
        
        if(bytesLeft > 0)
        {
            uint32_t index = freelistIndex(bytesLeft);
            ((Obj*)(mStartFree))->next = mFreeList[index];
            mFreeList[index] = (Obj*)mStartFree;
        }

        mStartFree = (char*)malloc(bytesToGet);

        mHeapSize += bytesToGet;
        mEndFree = mStartFree + bytesToGet;

        return chunkAlloc(size, nobjs);
    }
}