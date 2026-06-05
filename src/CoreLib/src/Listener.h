#pragma once

#include "Allocator.h"
#include "RawVector.h"
#include "types.h"

class CListenerBase;

class CListenerTargetBase {
public:
    CListenerTargetBase();
    CListenerTargetBase(const CListenerTargetBase& rhs);
    virtual ~CListenerTargetBase();

    CListenerTargetBase& operator=(const CListenerTargetBase& rhs);

    u32 GetNumListeners() const;
    CListenerBase* GetListenerBase(u32 idx) const;
    void Validate() const;
    bool IsListener(const CListenerBase* listener) const;
    void AddListener(CListenerBase* listener);
    void RemoveListener(CListenerBase* listener);

private:
    typedef CRawVector<CListenerBase*, CAllocatorMM> tListeners;
    tListeners Listeners;
};

class CListenerBase {
public:
    CListenerBase();
    CListenerBase(const CListenerBase& rhs);
    virtual ~CListenerBase();

    CListenerBase& operator=(const CListenerBase& rhs);

    bool IsSubscribed(const CListenerTargetBase* target) const;
    void Subscribe(CListenerTargetBase* target);
    void Unsubscribe(CListenerTargetBase* target);
    void UnsubscribeAll();
    void Validate() const;

private:
    typedef CRawVector<CListenerTargetBase*, CAllocatorMM> tSubscribed;
    tSubscribed Subscribed;
};

typedef char check_listener_target_base_size[sizeof(CListenerTargetBase) == 0x10 ? 1 : -1];
typedef char check_listener_base_size[sizeof(CListenerBase) == 0x10 ? 1 : -1];
