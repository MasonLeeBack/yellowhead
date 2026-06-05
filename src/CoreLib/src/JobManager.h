#pragma once

#include "types.h"

class CJobManager {
public:
    CJobManager(u32 num_threads);
    ~CJobManager();

    void AbortJobsForShutdown();
    u32 GetUniqueTag();
    u32 EnqueueJob(int priority, void (*function)(void*), void* user_data, u32 tag, const char* name);
    u32 CountJobsWithJobID(u32 job_id) const;
    u32 CountJobsWithTag(u32 tag) const;
    void WorkerThreadFunction();
    static void WorkerThreadFunctionStatic(u64 arg);

private:
    struct SJob {
        u32 JobID;
        void (*Function)(void*);
        void* UserData;
        u32 Tag;
        s32 Priority;
    };

    struct SJobIDMatches {
        u32 JobID;
    };

    u32 NextUniqueTag;
    u32 NextUniqueJobID;
    u8 Threads[0x10];
    u8 WorkQueue[0x40];
    u8 ActiveQueue[0x40];
};

extern CJobManager* GJobManager;
extern CJobManager* GHTTPJobManager;

typedef char check_job_manager_size[sizeof(CJobManager) == 0x98 ? 1 : -1];
