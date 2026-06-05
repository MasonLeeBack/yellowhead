#pragma once

#include "types.h"

#include <sys/synchronization.h>

class CMMSemaphore {
public:
    bool Increment(u32 count);
    bool WaitAndDecrement(int timeout);
    void DoAbort();
    bool Aborted() const { return Abort; }

private:
    sys_semaphore_t Sem;
    bool Abort;
};

typedef char check_cmm_semaphore_size[sizeof(CMMSemaphore) == 8 ? 1 : -1];
