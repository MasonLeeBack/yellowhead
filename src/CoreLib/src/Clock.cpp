#include "Clock.h"
#include <sys/sys_time.h>

// ============================================================================
// Globals
// ============================================================================

static u64 GStartTime;
static u64 GClockFreq;
static float GClockFreqInv;

// ============================================================================
// Functions
// ============================================================================

static void InitClock(void)
{
    GClockFreq = sys_time_get_timebase_frequency();

    if (GClockFreq != 0)
        GClockFreqInv = 1.0f / static_cast<float>(GClockFreq);
}

u64 GetClockFreq(void)
{
    if (likely(GClockFreq != 0))
        return GClockFreq;

    InitClock();
    return GClockFreq;
}

u64 GetClock(void)
{
    if (unlikely(GStartTime == 0)) {
        GStartTime = GetSystemClock();
    }

    return GetSystemClock() - GStartTime;
}

float GetClockSeconds(void)
{
    u64 clock = GetClock();

    if (unlikely(GClockFreq == 0))
        InitClock();

    return static_cast<float>(clock) * GClockFreqInv;
}
