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
    void* DirtyAlloc(u32 size);
    void DirtyFree();

private:
    void* Data;
    u32 Size;
    u32 Align;
};
