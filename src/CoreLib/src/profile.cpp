#include "Profile.h"

CProfileEntry GProfileRoot;
u32 CProfileName::Counter;
CVector<CProfileEntry, CAllocatorMM> GProfileEntryVector;
CVector<std::pair<u32, u32>, CAllocatorMM> GProfileEntryMap;
u32 GProfileFrameCount;
u32 GCurrentProfileEntry = 0xffffffffu;
u32 GProfileSpikeThreshold = 100000;

bool FindKey(const std::pair<u32, u32>& lhs, const std::pair<u32, u32>& rhs)
{
    return lhs.first < rhs.first;
}

CProfileName::CProfileName(const char* name) :
    Name(name),
    Id(Counter++)
{
}

CProfileEntry::CProfileEntry() :
    Time(0),
    AvgTime(0),
    PeakTime(0),
    Hits(0),
    Name(0),
    Key(0),
    Parent(0xffffffffu),
    FirstChild(0xffffffffu),
    NextSibling(0xffffffffu),
    Expanded(false)
{
}

void CProfileEntry::Init(CProfileName* name, u32 key, u32 parent)
{
    Time = 0;
    AvgTime = 0;
    PeakTime = 0;
    Hits = 0;
    Name = name;
    Key = key;
    Parent = parent;
    FirstChild = 0xffffffffu;
    NextSibling = 0xffffffffu;
    Expanded = false;
}

template <>
void CVector<std::pair<u32, u32>, CAllocatorMM>::try_reserve(u32 size)
{
    if (size > MaxSize)
        MaxSize = size;
}

template <>
std::pair<u32, u32>* CVector<std::pair<u32, u32>, CAllocatorMM>::insert(std::pair<u32, u32>* pos, const std::pair<u32, u32>& value)
{
    u32 idx = pos - Data;
    if (Size == MaxSize)
        try_reserve(Size + 1);
    for (u32 i = Size; i > idx; --i)
        Data[i] = Data[i - 1];
    Data[idx] = value;
    ++Size;
    return Data + idx;
}

void EndProfileFrame()
{
    ++GProfileFrameCount;
    GCurrentProfileEntry = 0;
}

template <>
void CVector<CProfileEntry, CAllocatorMM>::try_reserve(u32 size)
{
    if (size > MaxSize)
        MaxSize = size;
}

template <>
void CVector<CProfileEntry, CAllocatorMM>::try_resize(u32 size)
{
    if (size > MaxSize)
        try_reserve(size);
    Size = size;
}

bool CreateProfileEntry(CProfileName* name, u32& idx)
{
    idx = GProfileEntryVector.Size;
    if (GProfileEntryVector.Size == GProfileEntryVector.MaxSize)
        GProfileEntryVector.try_resize(GProfileEntryVector.Size + 1);
    else
        ++GProfileEntryVector.Size;
    GProfileEntryVector.Data[idx].Init(name, name ? name->Id : 0, GCurrentProfileEntry);
    return true;
}
