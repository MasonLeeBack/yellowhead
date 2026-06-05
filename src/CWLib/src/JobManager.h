#pragma once

class CJobManager {
public:
    CJobManager(unsigned int numThreads);
    ~CJobManager();

    void AbortJobsForShutdown();

private:
    struct SJob {
        u32 JobID;
        void (*Function)(void *);
        void *UserData;
        u32 Tag;
        s32 Priority;
    };

    u32 NextUniqueTag;
    u32 NextUniqueJobID;
    unsigned char Threads[0x10];
    unsigned char WorkQueue[0x40];
    unsigned char ActiveQueue[0x40];
};
