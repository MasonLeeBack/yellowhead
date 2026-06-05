#include "Allocator.h"

CAllocatorBucket* GAllTheBuckets[8];

CAllocatorBucket GOtherBucket(0);
CAllocatorBucket GVectorBucket(2);
CAllocatorBucket GSTLBucket(1);
CAllocatorBucket GContainerBucket(3);
CAllocatorBucket GGFXBucket(4);
CAllocatorBucket GBinkBucket(5);
CAllocatorBucket GResourceBucket(6);
CAllocatorBucket GProfileBucket(7);

CAllocatorBucket::CAllocatorBucket(u8 type)
{
    Type = type;
    Allocator = 0;
    GAllTheBuckets[type] = this;
}

CAllocatorBucket::~CAllocatorBucket()
{
    GAllTheBuckets[Type] = 0;
}

void CheckBuckets()
{
}

u32 CAllocatorMMAligned128::ResizePolicy(u32 size, u32 max_size, u32 element_size)
{
    u32 double_size = size * 2;
    if (element_size != 0) {
        u32 min_size = 32 / element_size;
        if (double_size < min_size)
            double_size = min_size;
    }

    if (max_size > double_size)
        return max_size;

    return double_size;
}

u32 CAllocatorMMAligned16::ResizePolicy(u32 size, u32 max_size, u32 element_size)
{
    u32 double_size = size * 2;
    if (element_size != 0) {
        u32 min_size = 32 / element_size;
        if (double_size < min_size)
            double_size = min_size;
    }

    if (max_size > double_size)
        return max_size;

    return double_size;
}

u32 CAllocatorMM::ResizePolicy(u32 size, u32 max_size, u32 element_size)
{
    u32 double_size = size * 2;
    if (element_size != 0) {
        u32 min_size = 32 / element_size;
        if (double_size < min_size)
            double_size = min_size;
    }

    if (max_size > double_size)
        return max_size;

    return double_size;
}

void CAllocatorMM::Free(CAllocatorBucket& bucket, void* ptr)
{
    bucket.Free(ptr);
}

void CAllocatorMMAligned128::Free(CAllocatorBucket& bucket, void* ptr)
{
    bucket.AlignedFree(ptr);
}

void CAllocatorMMAligned16::Free(CAllocatorBucket& bucket, void* ptr)
{
    bucket.AlignedFree(ptr);
}

void* CAllocatorMMAligned128::Malloc(CAllocatorBucket& bucket, u32 size)
{
    return bucket.AlignedMalloc(size, 128);
}

void* CAllocatorMM::Malloc(CAllocatorBucket& bucket, u32 size)
{
    return bucket.Malloc(size);
}

void* CAllocatorMMAligned128::Realloc(CAllocatorBucket& bucket, void* ptr, u32 size)
{
    return bucket.AlignedRealloc(ptr, size, 128);
}

void* CAllocatorMMAligned16::Realloc(CAllocatorBucket& bucket, void* ptr, u32 size)
{
    return bucket.AlignedRealloc(ptr, size, 16);
}

void* CAllocatorMM::Realloc(CAllocatorBucket& bucket, void* ptr, u32 size)
{
    return bucket.Realloc(ptr, size);
}
