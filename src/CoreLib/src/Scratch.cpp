#include "Scratch.h"

#include "Mem.h"
#include "mem_monitor.h"

u8* GOneFrameScratchPad;
u32 GCurScratchPad;
u32 GLastScratchPadSizeUsed;
u32 ScratchPadSize;

static u32 ScratchHeapMonID;
bool GWatchOut;

void ScratchPadClean()
{
    GOneFrameScratchPad = 0;
}

void ScratchPadReset()
{
    if (!GWatchOut) {
        HeapMon::OnSetHighwater(ScratchHeapMonID, GCurScratchPad);
        if (!GWatchOut)
            HeapMon::OnContainerReset(ScratchHeapMonID);
    }

    GLastScratchPadSizeUsed = GCurScratchPad;
    GCurScratchPad = 0;
}

bool ScratchPadInit()
{
    GLastScratchPadSizeUsed = 0;
    GCurScratchPad = 0;
    GOneFrameScratchPad = static_cast<u8*>(GSlabAlloc.ScratchPad.GetPtr());
    ScratchPadSize = GSlabAlloc.ScratchPad.GetSize();

    u32 heap_mon_id = 0;
    if (!GWatchOut)
        heap_mon_id = HeapMon::AllocateContainerID(GOneFrameScratchPad, ScratchPadSize, "ScratchPad");
    ScratchHeapMonID = heap_mon_id;

    return true;
}
