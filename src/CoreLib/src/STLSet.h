#pragma once

#include "mem_stl_buckets.h"

namespace std {

template <typename T>
struct less {
};

template <typename Traits>
struct _Tree_ptr {
    u8 _Mycomp[3];
};

template <typename Traits>
struct _Tree_val : public _Tree_ptr<Traits> {
    typename Traits::allocator_type _Alval;
};

template <typename K, typename Pr, typename Alloc, bool Mfl>
struct _Tset_traits {
    typedef K key_type;
    typedef K value_type;
    typedef Pr key_compare;
    typedef Alloc allocator_type;
};

template <typename Traits>
class _Tree : public _Tree_val<Traits> {
public:
    typedef void* pointer;
    typedef typename Traits::allocator_type::size_type size_type;

protected:
    pointer _Myhead;
    size_type _Mysize;
};

template <typename K, typename Pr = less<K>, typename Alloc = STLBucketAlloc<K> >
class set : public _Tree<_Tset_traits<K, Pr, Alloc, false> > {
public:
    typedef Alloc allocator_type;
};

} // namespace std

typedef char check_stl_set_size[sizeof(std::set<void*>) == 0x0c ? 1 : -1];
