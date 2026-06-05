#include "JobManager.h"

CJobManager* GJobManager;
CJobManager* GHTTPJobManager;

CJobManager::CJobManager(u32 num_threads) :
    NextUniqueTag(1),
    NextUniqueJobID(1),
    Threads(),
    WorkQueue(),
    ActiveQueue()
{
}

CJobManager::~CJobManager()
{
}

void CJobManager::AbortJobsForShutdown()
{
}

u32 CJobManager::GetUniqueTag()
{
    return NextUniqueTag++;
}

u32 CJobManager::EnqueueJob(int priority, void (*function)(void*), void* user_data, u32 tag, const char* name)
{
    return NextUniqueJobID++;
}

u32 CJobManager::CountJobsWithJobID(u32 job_id) const
{
    return 0;
}

u32 CJobManager::CountJobsWithTag(u32 tag) const
{
    return 0;
}

void CJobManager::WorkerThreadFunction()
{
}

void CJobManager::WorkerThreadFunctionStatic(u64 arg)
{
}
