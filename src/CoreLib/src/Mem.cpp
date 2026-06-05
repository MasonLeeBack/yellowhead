#include "Mem.h"

namespace MM {

void AlignedFree(void* ptr)
{
}

void Free(void* ptr)
{
}

void* AlignedRealloc(void* ptr, u32 size, u32 align)
{
    return 0;
}

void* AlignedMalloc(u32 size, u32 align)
{
    return 0;
}

void* Realloc(void* ptr, int size)
{
    return 0;
}

void* Malloc(int size)
{
    return 0;
}

}
