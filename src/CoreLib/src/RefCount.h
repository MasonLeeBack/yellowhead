#pragma once

#include "types.h"

class CBaseCounted {
public:
    CBaseCounted() : RefCount(0) {}
    CBaseCounted(const CBaseCounted& rhs) : RefCount(0) {}
    virtual ~CBaseCounted() {}

    CBaseCounted& operator=(const CBaseCounted& rhs) { return *this; }

    void AddRef() const { ++RefCount; }
    void Release() const { if (RefCount) --RefCount; }
    u32 GetRefCount() const { return RefCount; }

private:
    mutable u32 RefCount;
};

template <typename T>
class CP {
public:
    CP() : Ref(0) {}
    CP(T* ref) : Ref(ref) {}
    ~CP() {}

    T*& GetRefReferenceForSerialisation() { return Ref; }
    void AddRefForSerialisation() const {}
    T& operator*() const { return *Ref; }
    T* operator->() const { return Ref; }
    operator T*() const { return Ref; }
    T* GetRef() const { return Ref; }
    CP<T>& operator=(const T* ref) { Ref = const_cast<T*>(ref); return *this; }
    CP<T>& operator=(const CP<T>& rhs) { Ref = rhs.Ref; return *this; }
    bool operator!() const { return !Ref; }
    bool operator==(T* rhs) const { return Ref == rhs; }
    bool operator!=(T* rhs) const { return Ref != rhs; }
    void CopyFrom(const T* rhs) { Ref = const_cast<T*>(rhs); }

private:
    T* Ref;
};

typedef void (*RemoveRefFunc)(void*);

struct StaticCPForm {
    void* Ref;
    RemoveRefFunc RemoveRefPtr;
    StaticCPForm* NextPtr;
};

extern StaticCPForm* GStaticCPHead;

void CleanupStaticCP();

typedef char check_base_counted_size[sizeof(CBaseCounted) == 0x8 ? 1 : -1];
typedef char check_static_cp_form_size[sizeof(StaticCPForm) == 0xc ? 1 : -1];
