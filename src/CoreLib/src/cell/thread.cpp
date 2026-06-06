#include "types.h"
#include "thread.h"

#include <sys/ppu_thread.h>
#include <sys/timer.h>
#include <netex/net.h>

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

THREAD CreatePPUThread(THREADPROC threadproc, u64 thread_arg, const char* name, int priority, int stacksize, bool joinable)
{
    THREAD rv = 0;
    int ret = sys_ppu_thread_create(&rv, threadproc, thread_arg, priority, stacksize, joinable, name);
    if (ret != 0)
        return 0;
    return rv;
}

void ExitPPUThread(u64 retval)
{
    u64 thread_retval = retval;
    if (GsnGetLoadRequest)
        sys_net_free_thread_context(0, 1);
    sys_ppu_thread_exit(thread_retval);
}

bool JoinPPUThread(THREAD t, u64* thread_retval)
{
    u64 temp;

    if (t == 0)
        return false;
    if (thread_retval == 0)
        thread_retval = &temp;

    return sys_ppu_thread_join(t, thread_retval) == 0;
}

bool SetPPUThreadPriority(THREAD thread, int priority)
{
    int ret = sys_ppu_thread_set_priority(thread, priority);
    return ret == 0;
}

void ThreadSleep(int ms)
{
    if (ms == 0)
        sys_ppu_thread_yield();
    else
        sys_timer_usleep(ms * 1000);
}

void ThreadSleepUS(int us)
{
    if (us == 0)
        sys_ppu_thread_yield();
    else
        sys_timer_usleep(us);
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
