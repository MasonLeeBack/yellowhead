#pragma once

typedef u64 THREAD;
typedef u64 THREADID;
typedef void (*THREADPROC)(u64);

bool AmInMainThread(void);
bool AmInMainThreadOrSystemInit(void);
bool InitThreads(void);
bool AmInGameHostThread(void);
void SetGameHostThreadId(void);
void ClearGameHostThreadId(void);
THREAD GetCurrentPPUThread(void);
THREADID GetCurrentPPUThreadId(void);
THREAD CreatePPUThread(THREADPROC threadproc, u64 thread_arg, const char* name, int priority, int stacksize, bool joinable);
void ExitPPUThread(u64 retval);
bool JoinPPUThread(THREAD thread, u64* retval);
bool SetPPUThreadPriority(THREAD thread, int priority);
void ThreadSleep(int ms);
void ThreadSleepUS(int us);

extern volatile bool GsnGetLoadRequest;
