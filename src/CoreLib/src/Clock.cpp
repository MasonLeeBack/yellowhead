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

static inline float GetClockFreqInv(void)
{
    if (unlikely(GClockFreq == 0))
        InitClock();

    return GClockFreqInv;
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

    return static_cast<float>(clock) * GetClockFreqInv();
}

float ToSeconds(u64 clocktime)
{
    return static_cast<float>(clocktime) * GetClockFreqInv();
}

float ToMilliSeconds(u64 clocktime)
{
    return static_cast<float>(clocktime) * 1000.0f * GetClockFreqInv();
}

u64 ToMicroSecondsInt(u64 clocktime)
{
    float us = static_cast<float>(clocktime) * 1000.0f * 1000.0f * GetClockFreqInv();
    return static_cast<u64>(us);
}

bool InitPerformanceTimers(void)
{
    if (GClockFreq == 0)
        InitClock();

    return true;
}

u64 SecondsToClockTicks(float seconds)
{
    return static_cast<u64>(static_cast<float>(GetClockFreq()) * seconds);
}
