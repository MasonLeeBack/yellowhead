#pragma once

#include "Allocator.h"
#include "Directory.h"
#include "MMString.h"
#include "RawVector.h"
#include "TextRange.h"
#include "TextStream.h"
#include "types.h"

typedef int FileHandle;

class RLevel;

template <typename T>
class CResourceDescriptor {
private:
    u8 Data[0x28];
};

class CRandomStream {
private:
    u32 r0;
};

typedef u8 V_Slot[0xc];

class CMetric;

class CAuditResults {
public:
    CAuditResults();
    void Initialise(const CFilePath& path);
    void Begin(const char* name, char ch);
    void Finalise();
    void End(char ch);

private:
    FileHandle LogHandle;
    MMOTextStreamString Buffer;
    MMString<char> Scratch;
    MMString<char> Indent;
    CRawVector<u32, CAllocatorMM> ElementCountStack;
    u32 ElementCount;
};

class CLevelVisitor {
public:
    CLevelVisitor();
    void Initialise(u32 random_seed, u32 start_idx);

private:
    u32 TotalLevelCount;
    u32 CurrentLevelIdx;
    CResourceDescriptor<RLevel> CurrentLevel;
    V_Slot SlotsRemaining;
    CRandomStream RandomStream;
    bool RandomiseLevelOrder;
};

class CAuditState {
public:
    CAuditState();
    ~CAuditState();
    void Initialise(const char* filename);
    void Finalise();
    bool SelectNextLevel();
    bool PerFrameUpdate();
    void DisplayReport();

private:
    CLevelVisitor LevelVisitor;
    CAuditResults LevelResults;
    u32 StartLoadEvent;
    u32 FinishLoadEvent;
    u32 EndRunEvent;
    MMString<char> CurrentFilename;
    CRawVector<CMetric*, CAllocatorMM> Metrics;
};

typedef char check_audit_results_size[sizeof(CAuditResults) == 0x50 ? 1 : -1];
typedef char check_level_visitor_size[sizeof(CLevelVisitor) == 0x44 ? 1 : -1];
typedef char check_audit_state_size[sizeof(CAuditState) == 0xc8 ? 1 : -1];

class CIniSettings {
public:
    int GetInt(const char* name, int default_value) const;
    const char* GetString(const char* name, const char* default_value) const;
};

extern CIniSettings GIniSettings;

class CAuditMetricFactory {
public:
    static CMetric* CreateAudit(const char* name);
    static void CreateAll(CRawVector<CMetric*, CAllocatorMM>& metrics);
};

void Split(TextRange<char> range, char ch, CRawVector<TextRange<char>, CAllocatorMM>& ranges);
u32 StringLength(const char* str);

bool PerFrameUpdate();
void DisplayReport();
bool SelectNextLevel();

void SetSelectLevelHook(bool (*hook)());
void SetLevelExitHook(bool (*hook)());
void SetOnWorldReleaseHook(void (*hook)());
