#pragma once

#include "types.h"

template <typename T>
class CBaseVector {
public:
    u32 size() const { return Size; }
    T& operator[](u32 idx) { return Data[idx]; }
    const T& operator[](u32 idx) const { return Data[idx]; }

public:
    T* Data;
    u32 Size;
    u32 MaxSize;
};

template <typename T, typename Allocator>
class CRawVector {
public:
    CRawVector() : Data(0), Size(0), MaxSize(0) {}

    u32 size() const { return Size; }
    T& operator[](u32 idx) { return Data[idx]; }
    const T& operator[](u32 idx) const { return Data[idx]; }
    void try_reserve(u32 size);
    void try_resize(u32 size);

public:
    T* Data;
    u32 Size;
    u32 MaxSize;
};

template <typename T, typename Allocator>
class CVector {
public:
    u32 size() const { return Size; }
    T& operator[](u32 idx) { return Data[idx]; }
    const T& operator[](u32 idx) const { return Data[idx]; }
    void try_reserve(u32 size);
    void try_resize(u32 size);
    T* insert(T* pos, const T& value);

public:
    T* Data;
    u32 Size;
    u32 MaxSize;
};
