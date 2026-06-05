#pragma once

struct SCWLibOptions {
    SCWLibOptions();

    bool InitialiseForGame;
    bool InstallUnhandledExceptionFilter;
    bool InitialiseGfxMemPools;
    bool InitialiseSmallGfxMemPools;
    bool InitialiseAllocators;
    bool InitialiseNetwork;
};

struct CInitStep {
    const char *DebugText;
    bool *Check_This_Bool_Before_Init;
    bool (*InitFunc)();
    void (*CloseFunc)();
    bool (*PostResourceInitFunc)();
    bool Inited;
    CInitStep *ChainTo;
};

enum E_REGION {
    eREGION_SCEE = 0,
    eREGION_SCEE_BETA = 1,
    eREGION_SCEE_GOTY = 2,
    eREGION_SCEA = 3,
    eREGION_SCEA_BETA = 4,
    eREGION_SCEA_GOTY = 5,
    eREGION_SCEJ = 6,
    eREGION_SCEK = 7,
    eREGION_SCEK_GOTY = 8,
    eREGION_COUNT = 9,
};

class CTitleRegion {
public:
    CTitleRegion();
    int GetAppID() const;
    const char *GetServiceID() const;
    int GetRegionSubst() const;
    bool IsGOTYVersion() const;
    bool IsBetaVersion() const;
    void Init(const char *title);

protected:
    E_REGION Region;
};

void SetWantQuit(bool wantQuit);
bool IsKioskDemo();
u64 GetQuitTime();
bool IsDoingInitSteps();
bool WantReloadMainProfile();
bool WantQuitOrWantQuitRequested();
bool WantQuit();
void UpdateWantQuit();
void AddInitSteps(CInitStep *newsteps);
bool InitJobManager();
void AbortJobManager();
void CloseJobManager();
bool snTunerInitWrapper();
float TimeSinceSwap();
bool InitDebugVariables();
