#pragma once

#include "types.h"

namespace MM {
void* Malloc(int size);
void Free(void* ptr);
void* Realloc(void* ptr, int size);
void* AlignedMalloc(u32 size, u32 align);
void AlignedFree(void* ptr);
void* AlignedRealloc(void* ptr, u32 size, u32 align);
}

class CReservedMemory {
public:
    void* GetPtr() const { return Data; }
    u32 GetSize() const { return Size; }

    void* DirtyAlloc(u32 size);
    void DirtyFree();

private:
    void* Data;
    u32 Size;
    u32 Align;
};

class CSlabAlloc {
public:
    void* Data;
    CReservedMemory HostVideo;
    CReservedMemory GFX;
    CReservedMemory RenderTargets;
    CReservedMemory ScratchPad;
};

extern CSlabAlloc GSlabAlloc;

typedef char check_reserved_memory_size[sizeof(CReservedMemory) == 0xc ? 1 : -1];
