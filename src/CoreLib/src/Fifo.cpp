#include "types.h"

class CMMSemaphore {
public:
    void Increment(u32 count);
    bool WaitAndDecrement(int timeout);
    void DoAbort();
};

void CMMSemaphore::Increment(u32 count)
{
}

bool CMMSemaphore::WaitAndDecrement(int timeout)
{
    return false;
}

void CMMSemaphore::DoAbort()
{
}
