#pragma once

typedef u64 THREAD;
typedef u64 THREADID;

bool AmInMainThread(void);
bool AmInMainThreadOrSystemInit(void);
bool InitThreads(void);
bool AmInGameHostThread(void);
void SetGameHostThreadId(void);
void ClearGameHostThreadId(void);
THREAD GetCurrentPPUThread(void);
THREADID GetCurrentPPUThreadId(void);
