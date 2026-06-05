#include "Mem.h"

#include <stdlib.h>

static u32 MakeAligned(u32 size, u32 align)
{
    return (size + align - 1) & ~(align - 1);
}

static u32 Align(u32 size, u32 align)
{
    return MakeAligned(size, align);
}

void* CReservedMemory::DirtyAlloc(u32 size)
{
    if (Size < size)
        _Exit(1);

    return Data;
}

void CReservedMemory::DirtyFree()
{
}
