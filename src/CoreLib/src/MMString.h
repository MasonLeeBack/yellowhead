#pragma once

#include "Allocator.h"
#include "types.h"

#include <string.h>

s32 StringCompare(const char* lhs, const char* rhs);
s32 StringCompare(const wchar_t* lhs, const wchar_t* rhs);
s32 StringCompare(const tchar_t* lhs, const tchar_t* rhs);

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
    MMString<T>& operator+=(const T* rhs);
    MMString<T>& operator+=(const MMString<T>& rhs);
    MMString<T>& operator+=(T ch);
    void push_back(T ch);

    MMString<T>& insert(u32 pos, const MMString<T>& rhs, u32 subpos, u32 sublen);
    MMString<T>& insert(u32 pos, const MMString<T>& rhs);
    MMString<T>& insert(u32 pos, const T* str, u32 length);
    MMString<T>& insert(u32 pos, const T* str);

    MMString<T>& erase(u32 pos, u32 count);
    iterator erase(iterator pos);
    iterator erase(iterator first, iterator last);

    MMString<T>& replace(u32 pos, u32 count, const MMString<T>& rhs, u32 subpos, u32 sublen);
    MMString<T>& replace(u32 pos, u32 count, const MMString<T>& rhs);
    MMString<T>& replace(u32 pos, u32 count, const T* str, u32 length);
    MMString<T>& replace(u32 pos, u32 count, const T* str);

    MMString<T> substr(u32 pos, u32 count) const;
    void swap(MMString<T>& rhs);

    const T* c_str() const;
    size_type size() const;
    size_type length() const;
    size_type capacity() const;
    bool empty() const;
    void clear();
    void reserve(u32 capacity);
    void resize(u32 length, T ch);
    void resize(u32 length);

    const_iterator begin() const;
    iterator begin();
    const_iterator end() const;
    iterator end();

    T& operator[](u32 pos);
    const T& operator[](u32 pos) const;
    size_type find(const T* str, u32 pos, u32 count) const;

    s32 compare(const T* rhs) const;
    s32 compare(const MMString<T>& rhs) const;
    bool Contains(const T* str) const;
    bool eq(const T* rhs) const;
    bool eq(const MMString<T>& rhs) const;
    bool ne(const T* rhs) const;
    bool ne(const MMString<T>& rhs) const;
    bool lt(const T* rhs) const;
    bool lt(const MMString<T>& rhs) const;

private:
    static bool CanUseLocalData(u32 count);
    static T MakeLocalStoreFlag(u32 count);
    bool IsUsingLocalData() const;
    void Construct(const T* start, u32 length);
    void Terminate(u32 length);
    bool Grow(u32 capacity);

private:
    union {
        T LocalBuffer[LOCAL_STORE_BYTES / sizeof(T)];
        struct {
            T _LocalBufferPad[LOCAL_STORE_CHARS];
            T LocalStoreFlag;
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
inline void StringTraits<T>::Move(T* dst, const T* src, u32 count)
{
    memmove(dst, src, count * sizeof(T));
}

template <typename T>
inline void StringTraits<T>::Copy(T* dst, const T* src, u32 count)
{
    memcpy(dst, src, count * sizeof(T));
}

template <typename T>
inline __attribute__((noinline)) T* StringTraits<T>::Malloc(u32 count)
{
    return (T*)GOtherBucket.Malloc(count * sizeof(T));
}

template <typename T>
inline const T* StringTraits<T>::Find(const T* str, u32 count, T ch)
{
    for (u32 i = 0; i != count; ++i) {
        if (str[i] == ch)
            return str + i;
    }
    return 0;
}

template <typename T>
inline s32 StringTraits<T>::Compare(const T* lhs, const T* rhs, u32 count)
{
    return memcmp(lhs, rhs, count * sizeof(T));
}

template <typename T>
inline bool MMString<T>::CanUseLocalData(u32 count)
{
    return count <= LOCAL_STORE_CHARS;
}

template <typename T>
inline T MMString<T>::MakeLocalStoreFlag(u32 count)
{
    return (T)(LOCAL_STORE_CHARS - count);
}

template <typename T>
inline bool MMString<T>::IsUsingLocalData() const
{
    return LocalData.LocalStoreFlag != (T)-1;
}

template <typename T>
inline __attribute__((noinline)) void MMString<T>::Terminate(u32 length)
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
inline __attribute__((noinline)) void MMString<T>::Construct(const T* start, u32 length)
{
    if (CanUseLocalData(length)) {
        Traits::Move(LocalBuffer, start, length);
        LocalBuffer[length] = 0;
        LocalData.LocalStoreFlag = MakeLocalStoreFlag(length);
    } else {
        HeapData.Buffer = Traits::Malloc(length + 1);
        HeapData.Length = length;
        HeapData.Capacity = length;
        HeapData.Dummy = 0;
        Traits::Move(HeapData.Buffer, start, length);
        HeapData.Buffer[length] = 0;
    }
}

template <typename T>
inline __attribute__((noinline)) bool MMString<T>::Grow(u32 capacity)
{
    if (IsUsingLocalData()) {
        u32 length = size();
        if (CanUseLocalData(capacity) && MakeLocalStoreFlag(capacity) != (T)-1)
            return true;

        T* buffer = Traits::Malloc(capacity + 1);
        Traits::Copy(buffer, LocalBuffer, length);
        buffer[capacity] = 0;
        LocalData.LocalStoreFlag = (T)-1;
        HeapData.Buffer = buffer;
        HeapData.Length = length;
        HeapData.Capacity = capacity;
        return true;
    }

    if (HeapData.Capacity >= capacity)
        return true;

    T* old_buffer = HeapData.Buffer;
    T* buffer = Traits::Malloc(capacity + 1);
    Traits::Copy(buffer, old_buffer, HeapData.Length);
    buffer[capacity] = 0;
    GOtherBucket.Free(old_buffer);
    HeapData.Capacity = capacity;
    HeapData.Buffer = buffer;
    return true;
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
inline __attribute__((noinline)) MMString<T>& MMString<T>::assign(const MMString<T>& rhs)
{
    return assign(rhs.c_str(), rhs.size());
}

template <typename T>
inline MMString<T>& MMString<T>::assign(const T* str)
{
    return assign(str, MMStringLength(str));
}

template <typename T>
inline __attribute__((noinline)) MMString<T>& MMString<T>::assign(const T* start, u32 length)
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
inline __attribute__((noinline)) MMString<T>& MMString<T>::append(const T* start, u32 length)
{
    u32 old_length = size();
    u32 new_length = old_length + length;
    if (Grow(new_length)) {
        T* dst = begin();
        Traits::Copy(dst + old_length, start, length);
        Terminate(new_length);
    }
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
inline MMString<T>& MMString<T>::operator+=(const T* rhs)
{
    return append(rhs);
}

template <typename T>
inline MMString<T>& MMString<T>::operator+=(const MMString<T>& rhs)
{
    return append(rhs);
}

template <typename T>
inline MMString<T>& MMString<T>::operator+=(T ch)
{
    return append(ch);
}

template <typename T>
inline void MMString<T>::push_back(T ch)
{
    append(ch);
}

template <typename T>
inline __attribute__((noinline)) MMString<T>& MMString<T>::insert(u32 pos, const MMString<T>& rhs, u32 subpos, u32 sublen)
{
    u32 rhs_length = rhs.size();
    if (sublen == npos || subpos + sublen > rhs_length)
        sublen = rhs_length - subpos;

    if (sublen != 0) {
        u32 old_length = size();
        u32 new_length = old_length + sublen;
        if (Grow(new_length)) {
            T* dst = begin();
            const T* src = rhs.c_str();
            Traits::Move(dst + pos + sublen, dst + pos, old_length - pos);
            if (&rhs != this)
                Traits::Copy(dst + pos, src + subpos, sublen);
            else if (subpos <= pos)
                Traits::Move(dst + pos, dst + subpos, sublen);
            else
                Traits::Move(dst + pos, dst + subpos + sublen, sublen);
            Terminate(new_length);
        }
    }
    return *this;
}

template <typename T>
inline MMString<T>& MMString<T>::insert(u32 pos, const MMString<T>& rhs)
{
    return insert(pos, rhs, 0, npos);
}

template <typename T>
inline __attribute__((noinline)) MMString<T>& MMString<T>::insert(u32 pos, const T* str, u32 length)
{
    u32 old_length = size();
    u32 new_length = old_length + length;
    if (length != 0 && Grow(new_length)) {
        T* dst = begin();
        Traits::Move(dst + pos + length, dst + pos, old_length - pos);
        Traits::Copy(dst + pos, str, length);
        Terminate(new_length);
    }
    return *this;
}

template <typename T>
inline MMString<T>& MMString<T>::insert(u32 pos, const T* str)
{
    return insert(pos, str, MMStringLength(str));
}

template <typename T>
inline __attribute__((noinline)) MMString<T>& MMString<T>::erase(u32 pos, u32 count)
{
    u32 old_length = size();
    if (count == npos || pos + count > old_length)
        count = old_length - pos;

    if (count != 0) {
        T* dst = begin();
        Traits::Move(dst + pos, dst + pos + count, old_length - pos - count);
        Terminate(old_length - count);
    }
    return *this;
}

template <typename T>
inline typename MMString<T>::iterator MMString<T>::erase(iterator pos)
{
    u32 offset = pos - begin();
    erase(offset, 1);
    return begin() + offset;
}

template <typename T>
inline typename MMString<T>::iterator MMString<T>::erase(iterator first, iterator last)
{
    u32 offset = first - begin();
    erase(offset, last - first);
    return begin() + offset;
}

template <typename T>
inline __attribute__((noinline)) MMString<T>& MMString<T>::replace(u32 pos, u32 count, const MMString<T>& rhs, u32 subpos, u32 sublen)
{
    u32 rhs_length = rhs.size();
    if (sublen == npos || subpos + sublen > rhs_length)
        sublen = rhs_length - subpos;
    return replace(pos, count, rhs.c_str() + subpos, sublen);
}

template <typename T>
inline MMString<T>& MMString<T>::replace(u32 pos, u32 count, const MMString<T>& rhs)
{
    return replace(pos, count, rhs, 0, npos);
}

template <typename T>
inline __attribute__((noinline)) MMString<T>& MMString<T>::replace(u32 pos, u32 count, const T* str, u32 length)
{
    if (Contains(str))
        return replace(pos, count, *this, str - c_str(), length);

    u32 old_length = size();
    if (count == npos || pos + count > old_length)
        count = old_length - pos;

    u32 new_length = old_length + length - count;
    if ((count != 0 || length != 0) && Grow(new_length)) {
        T* dst = begin();
        if (count != length)
            Traits::Move(dst + pos + length, dst + pos + count, old_length - pos - count);
        Traits::Copy(dst + pos, str, length);
        Terminate(new_length);
    }
    return *this;
}

template <typename T>
inline MMString<T>& MMString<T>::replace(u32 pos, u32 count, const T* str)
{
    return replace(pos, count, str, MMStringLength(str));
}

template <typename T>
inline __attribute__((noinline)) MMString<T> MMString<T>::substr(u32 pos, u32 count) const
{
    u32 old_length = size();
    if (count == npos || pos + count > old_length)
        count = old_length - pos;
    return MMString<T>(c_str() + pos, count);
}

template <typename T>
inline void MMString<T>::swap(MMString<T>& rhs)
{
    u64 bits0 = Bits[0];
    u64 bits1 = Bits[1];
    Bits[0] = rhs.Bits[0];
    Bits[1] = rhs.Bits[1];
    rhs.Bits[0] = bits0;
    rhs.Bits[1] = bits1;
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
inline __attribute__((noinline)) void MMString<T>::clear()
{
    if (!IsUsingLocalData())
        GOtherBucket.Free(HeapData.Buffer);
    LocalBuffer[0] = 0;
    LocalData.LocalStoreFlag = MakeLocalStoreFlag(0);
}

template <typename T>
inline __attribute__((noinline)) void MMString<T>::reserve(u32 capacity)
{
    Grow(capacity);
}

template <typename T>
inline __attribute__((noinline)) void MMString<T>::resize(u32 length, T ch)
{
    u32 old_length = size();
    if (old_length > length) {
        Terminate(length);
    } else if (old_length < length && Grow(length)) {
        T* dst = begin();
        for (u32 i = old_length; i != length; ++i)
            dst[i] = ch;
        Terminate(length);
    }
}

template <typename T>
inline void MMString<T>::resize(u32 length)
{
    resize(length, 0);
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
inline T& MMString<T>::operator[](u32 pos)
{
    return begin()[pos];
}

template <typename T>
inline const T& MMString<T>::operator[](u32 pos) const
{
    return c_str()[pos];
}

template <typename T>
inline __attribute__((noinline)) typename MMString<T>::size_type MMString<T>::find(const T* str, u32 pos, u32 count) const
{
    u32 string_length = size();
    if (count == 0)
        return string_length < pos ? npos : pos;
    if (string_length <= pos || count > string_length - pos)
        return npos;

    const T* start = c_str();
    const T* cur = start + pos;
    const T* end = cur + (string_length - pos - count + 1);
    T ch = str[0];
    while (cur < end) {
        if (*cur == ch && Traits::Compare(cur, str, count) == 0)
            return cur - start;
        ++cur;
    }
    return npos;
}

template <typename T>
inline __attribute__((noinline)) s32 MMString<T>::compare(const T* rhs) const
{
    return StringCompare(c_str(), rhs);
}

template <typename T>
inline s32 MMString<T>::compare(const MMString<T>& rhs) const
{
    return compare(rhs.c_str());
}

template <typename T>
inline __attribute__((noinline)) bool MMString<T>::Contains(const T* str) const
{
    const T* start = c_str();
    return str >= start && str <= start + size();
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
