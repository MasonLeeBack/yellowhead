#include "Listener.h"

template <>
void CRawVector<CListenerBase*, CAllocatorMM>::try_reserve(u32 size)
{
    if (size > MaxSize)
        MaxSize = size;
}

template <>
void CRawVector<CListenerTargetBase*, CAllocatorMM>::try_reserve(u32 size)
{
    if (size > MaxSize)
        MaxSize = size;
}

CListenerTargetBase::CListenerTargetBase() :
    Listeners()
{
}

CListenerTargetBase::CListenerTargetBase(const CListenerTargetBase& rhs) :
    Listeners()
{
}

CListenerTargetBase::~CListenerTargetBase()
{
    for (u32 i = Listeners.Size; i != 0; --i) {
        if (Listeners.Data[i - 1])
            Listeners.Data[i - 1]->Unsubscribe(this);
    }
    CAllocatorMM::Free(GAllocatorMM, Listeners.Data);
}

CListenerTargetBase& CListenerTargetBase::operator=(const CListenerTargetBase& rhs)
{
    return *this;
}

u32 CListenerTargetBase::GetNumListeners() const
{
    return Listeners.Size;
}

CListenerBase* CListenerTargetBase::GetListenerBase(u32 idx) const
{
    return Listeners.Data[idx];
}

void CListenerTargetBase::Validate() const
{
}

bool CListenerTargetBase::IsListener(const CListenerBase* listener) const
{
    for (u32 i = 0; i < Listeners.Size; ++i) {
        if (Listeners.Data[i] == listener)
            return true;
    }
    return false;
}

void CListenerTargetBase::AddListener(CListenerBase* listener)
{
    if (IsListener(listener))
        return;
    if (Listeners.Size == Listeners.MaxSize)
        Listeners.try_reserve(Listeners.Size + 1);
    Listeners.Data[Listeners.Size++] = listener;
}

void CListenerTargetBase::RemoveListener(CListenerBase* listener)
{
    for (u32 i = 0; i < Listeners.Size; ++i) {
        if (Listeners.Data[i] == listener) {
            for (u32 j = i + 1; j < Listeners.Size; ++j)
                Listeners.Data[j - 1] = Listeners.Data[j];
            --Listeners.Size;
            return;
        }
    }
}

CListenerBase::CListenerBase() :
    Subscribed()
{
}

CListenerBase::CListenerBase(const CListenerBase& rhs) :
    Subscribed()
{
}

CListenerBase::~CListenerBase()
{
    UnsubscribeAll();
    CAllocatorMM::Free(GAllocatorMM, Subscribed.Data);
}

CListenerBase& CListenerBase::operator=(const CListenerBase& rhs)
{
    return *this;
}

bool CListenerBase::IsSubscribed(const CListenerTargetBase* target) const
{
    for (u32 i = 0; i < Subscribed.Size; ++i) {
        if (Subscribed.Data[i] == target)
            return true;
    }
    return false;
}

void CListenerBase::Subscribe(CListenerTargetBase* target)
{
    if (IsSubscribed(target))
        return;
    if (Subscribed.Size == Subscribed.MaxSize)
        Subscribed.try_reserve(Subscribed.Size + 1);
    Subscribed.Data[Subscribed.Size++] = target;
    target->AddListener(this);
}

void CListenerBase::Unsubscribe(CListenerTargetBase* target)
{
    for (u32 i = 0; i < Subscribed.Size; ++i) {
        if (Subscribed.Data[i] == target) {
            for (u32 j = i + 1; j < Subscribed.Size; ++j)
                Subscribed.Data[j - 1] = Subscribed.Data[j];
            --Subscribed.Size;
            target->RemoveListener(this);
            return;
        }
    }
}

void CListenerBase::UnsubscribeAll()
{
    while (Subscribed.Size)
        Unsubscribe(Subscribed.Data[Subscribed.Size - 1]);
}

void CListenerBase::Validate() const
{
}
