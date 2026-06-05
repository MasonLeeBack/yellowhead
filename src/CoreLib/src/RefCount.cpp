#include "RefCount.h"

StaticCPForm* GStaticCPHead;

void CleanupStaticCP()
{
    StaticCPForm* form = GStaticCPHead;
    while (form != 0) {
        form->RemoveRefPtr(form);
        form = form->NextPtr;
    }

    GStaticCPHead = 0;
}
