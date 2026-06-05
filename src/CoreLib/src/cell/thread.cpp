#include "types.h"
#include "thread.h"

#include <sys/ppu_thread.h>

bool GThreadInit;
sys_ppu_thread_t GGameHostThreadID;
bool GGameHostThreadInit;
sys_ppu_thread_t GMainThreadID;
THREAD g_init_thread_id;
volatile bool GsnGetLoadRequest;

THREAD GetCurrentPPUThread()
{
    THREAD rv = 0;
    int ret = sys_ppu_thread_get_id(&rv);
    (void)ret;
    return rv;
}

THREADID GetCurrentPPUThreadId()
{
    return GetCurrentPPUThread();
}

bool InitThreads()
{
    bool ret = true;
    if (!GThreadInit) {
        GThreadInit = true;
        ret = sys_ppu_thread_get_id(&GMainThreadID) == 0;
    }
    return ret;
}

bool AmInMainThread()
{
    sys_ppu_thread_t thread_id;
    s32 ret = sys_ppu_thread_get_id(&thread_id);
    (void)ret;

    if (!GThreadInit)
        return true;

    return thread_id == GMainThreadID;
}

bool AmInMainThreadOrSystemInit()
{
    if (AmInMainThread())
        return true;

    if (!GThreadInit)
        return true;

    return GetCurrentPPUThread() == g_init_thread_id;
}

void ClearGameHostThreadId()
{
    GGameHostThreadInit = false;
}

void SetGameHostThreadId()
{
    GGameHostThreadInit = true;
    s32 ret = sys_ppu_thread_get_id(&GGameHostThreadID);
    (void)ret;
}

bool AmInGameHostThread()
{
    sys_ppu_thread_t thread_id;
    s32 ret = sys_ppu_thread_get_id(&thread_id);
    (void)ret;

    if (!GGameHostThreadInit)
        return false;

    return thread_id == GGameHostThreadID;
}
