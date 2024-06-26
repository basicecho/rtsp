#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_

#include "Thread.h"
#include "Mutex.h"
#include "Condition.h"

#include <queue>
#include <vector>

class ThreadPool
{
public:
    class Task
    {
    public:
        typedef void (*TaskCallback)(void*);
        Task() { };
        
        void setTaskCallback(TaskCallback cb, void* arg) {
            mTaskCallback = cb; mArg = arg;
        }

        void handle() { 
            if(mTaskCallback) 
                mTaskCallback(mArg); 
        }

        bool operator=(const Task& task) {
            this->mTaskCallback = task.mTaskCallback;
            this->mArg = task.mArg;

            return true;
        }
    private:
        void (*mTaskCallback)(void*);
        void* mArg;
    };

    static ThreadPool* createNew(int num);
    
    ThreadPool(int num);
    ~ThreadPool();

    void addTask(Task& task);

private:
    class MThread : public Thread
    {
    protected:
        virtual void run(void *arg);
    };

    void createThreads();
    void cancelThreads();
    void handleTask();

private:
    std::queue<Task> mTaskQueue;
    Mutex* mMutex;
    Condition* mCondition;
    std::vector<MThread> mThreads;
    bool mQuit;
};

#endif