#include "HttpClient.h"

#include "DebugLog.h"

#include <cell/atomic.h>
#include <ppu_intrinsics.h>

void GetAvailableBandwidthToHTTP(float& availableIn, float& availableOut);

volatile u32 GNumHTTPTaskBaseWorkers;
CCriticalSec LibHTTPCS;
CHTTPClient* GHTTPClient;
bool LibHTTPInited;
void* LibHTTPPool;
void* LibSSLPool;

float GetTimeThrottledTaskShouldTake(u32 bytesIn, u32 bytesOut)
{
    float availableIn;
    float availableOut;
    GetAvailableBandwidthToHTTP(availableIn, availableOut);

    availableIn *= 0.5f;
    availableOut *= 0.5f;
    float timeIn = (float)bytesIn / availableIn;
    float timeOut = (float)bytesOut / availableOut;
    float time = __fsels(timeIn - timeOut, timeIn, timeOut);
    time = __fsels(time - 0.1f, 0.1f, time);

    DebugLogChF(DC_HTTP, "desired_bandwidth_usage %f => time_between_sending_blocks %f\n",
                bytesIn, bytesOut, timeIn, timeOut);
    return time;
}

u32 GetNumHTTPTaskBaseWorkers()
{
    return GNumHTTPTaskBaseWorkers;
}

CHttpWorkCounter::CHttpWorkCounter()
{
    cellAtomicIncr32((u32*)&GNumHTTPTaskBaseWorkers);
}

CHttpWorkCounter::~CHttpWorkCounter()
{
    cellAtomicDecr32((u32*)&GNumHTTPTaskBaseWorkers);
}
