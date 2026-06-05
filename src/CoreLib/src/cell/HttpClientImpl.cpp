#include "HttpClient.h"

#include <cell/atomic.h>

u32 GNumHTTPTaskBaseWorkers;

u32 GetNumHTTPTaskBaseWorkers()
{
    return GNumHTTPTaskBaseWorkers;
}

CHttpWorkCounter::CHttpWorkCounter()
{
    cellAtomicIncr32(&GNumHTTPTaskBaseWorkers);
}

CHttpWorkCounter::~CHttpWorkCounter()
{
    cellAtomicDecr32(&GNumHTTPTaskBaseWorkers);
}
