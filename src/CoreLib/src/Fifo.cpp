#include "Fifo.h"

bool CMMSemaphore::Increment(u32 count)
{
    bool ret = false;
    if (!Abort) {
        if (sys_semaphore_post(Sem, count) == 0)
            ret = Abort == 0;
    }
    return ret;
}

bool CMMSemaphore::WaitAndDecrement(int timeout)
{
    bool ok = false;
    int ret;
    if (timeout == 0) {
        ret = sys_semaphore_trywait(Sem);
    } else {
        usecond_t wait = 0;
        if (timeout != -1)
            wait = timeout * 1000;
        ret = sys_semaphore_wait(Sem, wait);
    }
    if (ret == 0)
        ok = Abort == 0;
    return ok;
}

void CMMSemaphore::DoAbort()
{
    if (!Abort) {
        Abort = true;
        sys_semaphore_post(Sem, 1000);
    }
}
