#include "Mem.h"

#include <stdlib.h>

namespace MM {

void AlignedFree(void* ptr)
{
    free(ptr);
}

void Free(void* ptr)
{
    free(ptr);
}

void* AlignedRealloc(void* ptr, u32 size, u32 align)
{
    return reallocalign(ptr, size, align);
}

void* AlignedMalloc(u32 size, u32 align)
{
    return memalign(align, size);
}

void* Realloc(void* ptr, int size)
{
    return realloc(ptr, size);
}

void* Malloc(int size)
{
    return malloc(size);
}

}
