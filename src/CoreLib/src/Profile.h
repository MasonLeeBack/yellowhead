#pragma once

#include "Allocator.h"
#include "RawVector.h"
#include "types.h"

#include <utility>

class CProfileName {
public:
    CProfileName(const char* name);

    const char* Name;
    u32 Id;

    static u32 Counter;
};

class CProfileEntry {
public:
    CProfileEntry();
    ~CProfileEntry() {}

    void Init(CProfileName* name, u32 key, u32 parent);

    u64 Time;
    u64 AvgTime;
    u64 PeakTime;
    u64 Hits;
    CProfileName* Name;
    u32 Key;
    u32 Parent;
    u32 FirstChild;
    u32 NextSibling;
    bool Expanded;
};

extern CVector<std::pair<u32, u32>, CAllocatorMM> GProfileEntryMap;
extern u32 GCurrentProfileEntry;
extern CVector<CProfileEntry, CAllocatorMM> GProfileEntryVector;
extern CProfileEntry GProfileRoot;
extern u32 GProfileFrameCount;
extern u32 GProfileSpikeThreshold;

bool FindKey(const std::pair<u32, u32>& lhs, const std::pair<u32, u32>& rhs);
void EndProfileFrame();
bool CreateProfileEntry(CProfileName* name, u32& idx);

typedef char check_profile_name_size[sizeof(CProfileName) == 0x8 ? 1 : -1];
typedef char check_profile_entry_size[sizeof(CProfileEntry) == 0x38 ? 1 : -1];
