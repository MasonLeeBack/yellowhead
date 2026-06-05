#pragma once

#include "types.h"

#include <sys/synchronization.h>

class CCriticalSec {
public:
    sys_lwmutex_t cs;
    const char* Name;
    const char* LockFile;
    int LockLine;
    int DEBUGIsLocked;
};

typedef char check_critical_sec_size[sizeof(CCriticalSec) == 0x28 ? 1 : -1];
