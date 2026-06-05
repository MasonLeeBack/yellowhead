#pragma once

#include "types.h"

template <typename T>
class STLBucketAlloc {
public:
    typedef T* pointer;
    typedef const T* const_pointer;
    typedef T& reference;
    typedef const T& const_reference;
    typedef u32 size_type;
    typedef s32 difference_type;
    typedef T value_type;

    template <typename U>
    struct rebind {
        typedef STLBucketAlloc<U> other;
    };
};

typedef char check_stl_bucket_alloc_size[sizeof(STLBucketAlloc<void*>) == 0x1 ? 1 : -1];
