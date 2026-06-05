#pragma once

#include "types.h"

// ============================================================================
// Functions
// ============================================================================

inline u64 GetSystemClock(void)
{
    u64 result;
    __asm__ volatile("mftb %0" : "=r"(result) : : "memory");
    return result;
}

u64 GetClock(void);
u64 GetClockFreq(void);
float ToSeconds(u64 clocktime);
float ToMilliSeconds(u64 clocktime);
float GetClockSeconds(void);
