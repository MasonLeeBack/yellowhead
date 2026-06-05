#pragma once

#include "Allocator.h"
#include "types.h"

template <typename T>
class StringTraits {
public:
    static void Move(T* dst, const T* src, u32 count);
    static void Copy(T* dst, const T* src, u32 count);
    static T* Malloc(u32 count);
    static const T* Find(const T* str, u32 count, T ch);
    static s32 Compare(const T* lhs, const T* rhs, u32 count);
};

template <typename T>
class MMString {
public:
    typedef StringTraits<T> Traits;
    typedef u32 size_type;
    typedef T* iterator;
    typedef const T* const_iterator;

    static const size_type npos = (size_type)-1;
    static const u32 LOCAL_STORE_BYTES = 0x10;
    static const u32 LOCAL_STORE_CHARS = LOCAL_STORE_BYTES / sizeof(T) - 1;

    MMString();
    MMString(const MMString<T>& rhs);
    MMString(const T* str);
    MMString(const T* str, u32 length);
    MMString(const T* start, const T* end);
    ~MMString();

    MMString<T>& operator=(const MMString<T>& rhs);
    MMString<T>& operator=(const T* str);
    MMString<T>& operator=(T ch);

    MMString<T>& assign(const MMString<T>& rhs);
    MMString<T>& assign(const T* str);
    MMString<T>& assign(const T* start, u32 length);
    MMString<T>& assign(const T* start, const T* end);
    MMString<T>& assign(T ch);

    MMString<T>& append(const MMString<T>& rhs);
    MMString<T>& append(const T* str);
    MMString<T>& append(const T* start, u32 length);
    MMString<T>& append(const T* start, const T* end);
    MMString<T>& append(T ch);
    void push_back(T ch);

    const T* c_str() const;
    size_type size() const;
    size_type length() const;
    size_type capacity() const;
    bool empty() const;
    void clear();

    const_iterator begin() const;
    iterator begin();
    const_iterator end() const;
    iterator end();

    s32 compare(const T* rhs) const;
    s32 compare(const MMString<T>& rhs) const;
    bool eq(const T* rhs) const;
    bool eq(const MMString<T>& rhs) const;
    bool ne(const T* rhs) const;
    bool ne(const MMString<T>& rhs) const;
    bool lt(const T* rhs) const;
    bool lt(const MMString<T>& rhs) const;

private:
    static bool CanUseLocalData(u32 count);
    static u8 MakeLocalStoreFlag(u32 count);
    bool IsUsingLocalData() const;
    void Construct(const T* start, u32 length);
    void Terminate(u32 length);

private:
    union {
        T LocalBuffer[LOCAL_STORE_BYTES / sizeof(T)];
        struct {
            u8 _LocalBufferPad[LOCAL_STORE_BYTES - 1];
            u8 LocalStoreFlag;
        } LocalData;
        struct {
            T* Buffer;
            size_type Length;
            size_type Capacity;
            u32 Dummy;
        } HeapData;
        u64 Bits[2];
    };
} __attribute__((aligned(8)));

template <typename T>
inline u32 MMStringLength(const T* str)
{
    const T* cur = str;
    while (*cur)
        ++cur;
    return cur - str;
}

template <typename T>
inline bool MMString<T>::CanUseLocalData(u32 count)
{
    return count <= LOCAL_STORE_CHARS;
}

template <typename T>
inline u8 MMString<T>::MakeLocalStoreFlag(u32 count)
{
    return (u8)(LOCAL_STORE_CHARS - count);
}

template <typename T>
inline bool MMString<T>::IsUsingLocalData() const
{
    return LocalData.LocalStoreFlag != 0xff;
}

template <typename T>
inline void MMString<T>::Terminate(u32 length)
{
    if (IsUsingLocalData()) {
        LocalBuffer[length] = 0;
        LocalData.LocalStoreFlag = MakeLocalStoreFlag(length);
    } else {
        HeapData.Buffer[length] = 0;
        HeapData.Length = length;
    }
}

template <typename T>
inline void MMString<T>::Construct(const T* start, u32 length)
{
    if (CanUseLocalData(length)) {
        for (u32 i = 0; i != length; ++i)
            LocalBuffer[i] = start[i];
        LocalBuffer[length] = 0;
        LocalData.LocalStoreFlag = MakeLocalStoreFlag(length);
    } else {
        HeapData.Buffer = (T*)GOtherBucket.Malloc((length + 1) * sizeof(T));
        HeapData.Length = length;
        HeapData.Capacity = length;
        HeapData.Dummy = 0;
        for (u32 i = 0; i != length; ++i)
            HeapData.Buffer[i] = start[i];
        HeapData.Buffer[length] = 0;
    }
}

template <typename T>
inline MMString<T>::MMString()
{
    LocalBuffer[0] = 0;
    LocalData.LocalStoreFlag = MakeLocalStoreFlag(0);
}

template <typename T>
inline MMString<T>::MMString(const MMString<T>& rhs)
{
    Construct(rhs.c_str(), rhs.size());
}

template <typename T>
inline MMString<T>::MMString(const T* str)
{
    Construct(str, MMStringLength(str));
}

template <typename T>
inline MMString<T>::MMString(const T* str, u32 length)
{
    Construct(str, length);
}

template <typename T>
inline MMString<T>::MMString(const T* start, const T* end)
{
    Construct(start, end - start);
}

template <typename T>
inline MMString<T>::~MMString()
{
    if (!IsUsingLocalData())
        GOtherBucket.Free(HeapData.Buffer);
}

template <typename T>
inline MMString<T>& MMString<T>::operator=(const MMString<T>& rhs)
{
    return assign(rhs);
}

template <typename T>
inline MMString<T>& MMString<T>::operator=(const T* str)
{
    return assign(str);
}

template <typename T>
inline MMString<T>& MMString<T>::operator=(T ch)
{
    return assign(ch);
}

template <typename T>
inline MMString<T>& MMString<T>::assign(const MMString<T>& rhs)
{
    return assign(rhs.c_str(), rhs.size());
}

template <typename T>
inline MMString<T>& MMString<T>::assign(const T* str)
{
    return assign(str, MMStringLength(str));
}

template <typename T>
inline MMString<T>& MMString<T>::assign(const T* start, u32 length)
{
    if (!IsUsingLocalData())
        GOtherBucket.Free(HeapData.Buffer);
    Construct(start, length);
    return *this;
}

template <typename T>
inline MMString<T>& MMString<T>::assign(const T* start, const T* end)
{
    return assign(start, end - start);
}

template <typename T>
inline MMString<T>& MMString<T>::assign(T ch)
{
    return assign(&ch, 1);
}

template <typename T>
inline MMString<T>& MMString<T>::append(const MMString<T>& rhs)
{
    return append(rhs.c_str(), rhs.size());
}

template <typename T>
inline MMString<T>& MMString<T>::append(const T* str)
{
    return append(str, MMStringLength(str));
}

template <typename T>
inline MMString<T>& MMString<T>::append(const T* start, u32 length)
{
    u32 old_length = size();
    u32 new_length = old_length + length;
    T* tmp = (T*)GOtherBucket.Malloc((new_length + 1) * sizeof(T));
    const T* old = c_str();
    for (u32 i = 0; i != old_length; ++i)
        tmp[i] = old[i];
    for (u32 i = 0; i != length; ++i)
        tmp[old_length + i] = start[i];
    tmp[new_length] = 0;
    assign(tmp, new_length);
    GOtherBucket.Free(tmp);
    return *this;
}

template <typename T>
inline MMString<T>& MMString<T>::append(const T* start, const T* end)
{
    return append(start, end - start);
}

template <typename T>
inline MMString<T>& MMString<T>::append(T ch)
{
    return append(&ch, 1);
}

template <typename T>
inline void MMString<T>::push_back(T ch)
{
    append(ch);
}

template <typename T>
inline const T* MMString<T>::c_str() const
{
    return IsUsingLocalData() ? LocalBuffer : HeapData.Buffer;
}

template <typename T>
inline typename MMString<T>::size_type MMString<T>::size() const
{
    return IsUsingLocalData() ? LOCAL_STORE_CHARS - LocalData.LocalStoreFlag : HeapData.Length;
}

template <typename T>
inline typename MMString<T>::size_type MMString<T>::length() const
{
    return size();
}

template <typename T>
inline typename MMString<T>::size_type MMString<T>::capacity() const
{
    return IsUsingLocalData() ? LOCAL_STORE_CHARS : HeapData.Capacity;
}

template <typename T>
inline bool MMString<T>::empty() const
{
    return size() == 0;
}

template <typename T>
inline void MMString<T>::clear()
{
    if (!IsUsingLocalData())
        GOtherBucket.Free(HeapData.Buffer);
    LocalBuffer[0] = 0;
    LocalData.LocalStoreFlag = MakeLocalStoreFlag(0);
}

template <typename T>
inline typename MMString<T>::const_iterator MMString<T>::begin() const
{
    return c_str();
}

template <typename T>
inline typename MMString<T>::iterator MMString<T>::begin()
{
    return IsUsingLocalData() ? LocalBuffer : HeapData.Buffer;
}

template <typename T>
inline typename MMString<T>::const_iterator MMString<T>::end() const
{
    return c_str() + size();
}

template <typename T>
inline typename MMString<T>::iterator MMString<T>::end()
{
    return begin() + size();
}

template <typename T>
inline s32 MMString<T>::compare(const T* rhs) const
{
    const T* lhs = c_str();
    while (*lhs && *lhs == *rhs) {
        ++lhs;
        ++rhs;
    }
    return (s32)*lhs - (s32)*rhs;
}

template <typename T>
inline s32 MMString<T>::compare(const MMString<T>& rhs) const
{
    return compare(rhs.c_str());
}

template <typename T>
inline bool MMString<T>::eq(const T* rhs) const { return compare(rhs) == 0; }

template <typename T>
inline bool MMString<T>::eq(const MMString<T>& rhs) const { return compare(rhs) == 0; }

template <typename T>
inline bool MMString<T>::ne(const T* rhs) const { return compare(rhs) != 0; }

template <typename T>
inline bool MMString<T>::ne(const MMString<T>& rhs) const { return compare(rhs) != 0; }

template <typename T>
inline bool MMString<T>::lt(const T* rhs) const { return compare(rhs) < 0; }

template <typename T>
inline bool MMString<T>::lt(const MMString<T>& rhs) const { return compare(rhs) < 0; }
