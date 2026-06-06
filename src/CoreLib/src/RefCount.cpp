#include "RefCount.h"

StaticCPForm* GStaticCPHead;

void CleanupStaticCP()
{
    StaticCPForm* next = GStaticCPHead;
    while (next != 0) {
        next->RemoveRefPtr(next);
        next = next->NextPtr;
    }

    GStaticCPHead = 0;
}
