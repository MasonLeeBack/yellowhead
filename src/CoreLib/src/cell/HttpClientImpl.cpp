#include "HttpClient.h"

#include <cell/atomic.h>

volatile u32 GNumHTTPTaskBaseWorkers;
CCriticalSec LibHTTPCS;
CHTTPClient* GHTTPClient;
bool LibHTTPInited;
void* LibHTTPPool;
void* LibSSLPool;

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
