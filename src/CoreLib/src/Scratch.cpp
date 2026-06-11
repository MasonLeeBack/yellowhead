#include "Scratch.h"

#include "Mem.h"
#include "mem_monitor.h"

u8* GOneFrameScratchPad;
static u32 ScratchHeapMonID;
u32 GCurScratchPad;
u32 GLastScratchPadSizeUsed;
u32 ScratchPadSize;
s32 GWatchOut;

void ScratchPadClean()
{
    GOneFrameScratchPad = 0;
}

void ScratchPadReset()
{
    if (HeapMon::GHeapMonMode < HeapMon::HMM_BUFFERING)
    {
        GLastScratchPadSizeUsed = GCurScratchPad;
        GCurScratchPad = 0;
    }
    else
    {
        HeapMon::OnSetHighwater(ScratchHeapMonID, GCurScratchPad);
        if (HeapMon::GHeapMonMode >= HeapMon::HMM_BUFFERING)
            HeapMon::OnContainerReset(ScratchHeapMonID);

        GLastScratchPadSizeUsed = GCurScratchPad;
        GCurScratchPad = 0;
    }
}

bool ScratchPadInit()
{
    u32 ContainerID = 0;

    GLastScratchPadSizeUsed = 0;
    GCurScratchPad = 0;

    GOneFrameScratchPad = (u8*)GSlabAlloc.ScratchPad.GetPtr();
    ScratchPadSize = GSlabAlloc.ScratchPad.GetSize();

    if (HeapMon::GHeapMonMode >= HeapMon::HMM_BUFFERING)
        ContainerID = HeapMon::AllocateContainerID(GSlabAlloc.ScratchPad.GetPtr(), GSlabAlloc.ScratchPad.GetSize(), "ScratchPad");

    ScratchHeapMonID = ContainerID;

    return true;
}
