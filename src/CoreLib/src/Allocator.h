#pragma once

#include "types.h"

class CAllocatorBucket {
public:
    explicit CAllocatorBucket(u8 type);
    ~CAllocatorBucket();

    void Check();
    u32 Size();
    void* Malloc(u32 size);
    void Free(void* ptr);
    void* Realloc(void* ptr, u32 size);
    void* AlignedMalloc(u32 size, u32 alignment);
    void AlignedFree(void* ptr);
    void* AlignedRealloc(void* ptr, u32 size, u32 alignment);

private:
    u8 Timer[48];
    void* Allocator;
    u8 Type;
    u8 Padding[3];
};

class CAllocatorMM {
public:
    static void* Malloc(CAllocatorBucket& bucket, u32 size);
    static void Free(CAllocatorBucket& bucket, void* ptr);
    static void* Realloc(CAllocatorBucket& bucket, void* ptr, u32 size);
    static u32 ResizePolicy(u32 size, u32 max_size, u32 element_size);
};

class CAllocatorMMAligned128 {
public:
    static void* Malloc(CAllocatorBucket& bucket, u32 size);
    static void Free(CAllocatorBucket& bucket, void* ptr);
    static void* Realloc(CAllocatorBucket& bucket, void* ptr, u32 size);
    static u32 ResizePolicy(u32 size, u32 max_size, u32 element_size);
};

class CAllocatorMMAligned16 {
public:
    static void* Malloc(CAllocatorBucket& bucket, u32 size);
    static void Free(CAllocatorBucket& bucket, void* ptr);
    static void* Realloc(CAllocatorBucket& bucket, void* ptr, u32 size);
    static u32 ResizePolicy(u32 size, u32 max_size, u32 element_size);
};

extern CAllocatorBucket GOtherBucket;
extern CAllocatorBucket GVectorBucket;
extern CAllocatorBucket GSTLBucket;
extern CAllocatorBucket GContainerBucket;
extern CAllocatorBucket GGFXBucket;
extern CAllocatorBucket GBinkBucket;
extern CAllocatorBucket GResourceBucket;
extern CAllocatorBucket GProfileBucket;

#define GAllocatorMM GVectorBucket

typedef char check_allocator_bucket_size[sizeof(CAllocatorBucket) == 0x38 ? 1 : -1];
typedef char check_allocator_mm_size[sizeof(CAllocatorMM) == 0x1 ? 1 : -1];
typedef char check_allocator_mm_aligned_16_size[sizeof(CAllocatorMMAligned16) == 0x1 ? 1 : -1];
typedef char check_allocator_mm_aligned_128_size[sizeof(CAllocatorMMAligned128) == 0x1 ? 1 : -1];

inline void* operator new(unsigned int size)
{
    return GOtherBucket.Malloc(size);
}

inline void operator delete(void* ptr)
{
    GOtherBucket.Free(ptr);
}

inline void* operator new(unsigned int, void* ptr)
{
    return ptr;
}
