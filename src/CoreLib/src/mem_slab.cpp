#include "Mem.h"

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
    Size = size;
    return Data;
}

void CReservedMemory::DirtyFree()
{
    Data = 0;
    Size = 0;
}
