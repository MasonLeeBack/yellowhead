#include "pch.h"

u64 GSetWantQuitTime;
bool GIsDoingInitSteps;
bool GWantReloadMainProfile;
namespace {
SCWLibOptions GOptions;
}
CTitleRegion GTitleRegion;
bool GWantQuitRequested;
bool GWantQuit;
u64 GCheckWantQuitTime;
u64 GCheckWantQuitOrWantQuitRequestedTime;
u64 GUpdateWantQuitTime;
bool GWantReboot;

void SetWantQuit(bool wantQuit)
{
    if (wantQuit && !GWantQuitRequested) {
        GSetWantQuitTime = GetClock();

        float setWantQuitSeconds = ToSeconds(GSetWantQuitTime);
        float clockSeconds = GetClockSeconds();
        DebugLogChF(DC_INIT, "SetWantQuit() at %.2f %.2f", setWantQuitSeconds, clockSeconds);
    }

    GWantQuitRequested = wantQuit;
}

u64 GetQuitTime()
{
    return GSetWantQuitTime;
}

bool WantQuit()
{
    if (AmInMainThread())
        GCheckWantQuitTime = GetClock();

    return GWantQuit;
}

bool IsDoingInitSteps()
{
    return GIsDoingInitSteps;
}

bool WantQuitOrWantQuitRequested()
{
    if (!AmInMainThread())
        return GWantQuitRequested || GWantQuit;

    GCheckWantQuitOrWantQuitRequestedTime = GetClock();

    return GWantQuitRequested || GWantQuit;
}

bool WantReloadMainProfile()
{
    return GWantReloadMainProfile;
}

void UpdateWantQuit()
{
    if (AmInMainThread())
        GUpdateWantQuitTime = GetClock();

    if (GWantQuitRequested) {
        if (GWantQuit)
            return;

        if (!WantReloadMainProfile())
            CHTTPClient::Abort();

        CGameHostState::SetGameHostThreadToLowPriority();
        CSaveGameUtil::QuitGame();

        float setWantQuitSeconds = ToSeconds(GSetWantQuitTime);
        float clockSeconds = GetClockSeconds();
        DebugLogChF(DC_INIT, "UpdateWantQuit() at %.2f %.2f", setWantQuitSeconds, clockSeconds);

        GWantQuit = true;
        return;
    }

    if (!GWantQuit)
        return;

    GWantQuit = false;
    GSetWantQuitTime = 0;
}

SCWLibOptions::SCWLibOptions()
    : InitialiseForGame(false),
      InstallUnhandledExceptionFilter(true),
      InitialiseGfxMemPools(true),
      InitialiseSmallGfxMemPools(false),
      InitialiseAllocators(true),
      InitialiseNetwork(true)
{
}

extern CInitStep GInitSteps[];

void AddInitSteps(CInitStep *newsteps)
{
    CInitStep *last = 0;
    CInitStep *step = GInitSteps;

    while (step->InitFunc || step->CloseFunc || step->PostResourceInitFunc) {
        last = step;
        if (step->ChainTo)
            step = step->ChainTo;
        else
            step++;
    }

    last->ChainTo = newsteps;
}

extern s64 GLastSwapTime;

float TimeSinceSwap()
{
    s64 c = GetClock() - GLastSwapTime;
    if (c < 0)
        c = 0;

    return ToMilliSeconds(c);
}

bool snTunerInitWrapper()
{
    return true;
}

bool InitDebugVariables()
{
    CDebugVariable::AddAllToDebugRegistry();
    return true;
}

extern CJobManager *GJobManager;
extern CJobManager *GHTTPJobManager;

bool InitJobManager()
{
    GJobManager = new CJobManager(4);
    GHTTPJobManager = new CJobManager(2);

    return true;
}

void AbortJobManager()
{
    if (GJobManager)
        GJobManager->AbortJobsForShutdown();

    if (GHTTPJobManager)
        GHTTPJobManager->AbortJobsForShutdown();
}

void CloseJobManager()
{
    delete GJobManager;
    GJobManager = 0;

    delete GHTTPJobManager;
    GHTTPJobManager = 0;
}

extern const char *GTitleIDs[eREGION_COUNT];
extern "C" int strcmp(const char *lhs, const char *rhs);
extern "C" int printf(const char *fmt, ...);
void CycleTitleID();

CTitleRegion::CTitleRegion()
    : Region(eREGION_SCEE)
{
}

int CTitleRegion::GetAppID() const
{
    const int APPID_SCEE = 0x53b0;
    const int APPID_SCEA = 0x53a6;
    const int APPID_SCEJ = 0x53ba;
    const int APPID_SCEK = 0x53c4;

    switch (Region) {
    case eREGION_SCEA:
    case eREGION_SCEA_BETA:
    case eREGION_SCEA_GOTY:
        return APPID_SCEA;
    case eREGION_SCEJ:
        return APPID_SCEJ;
    case eREGION_SCEK:
    case eREGION_SCEK_GOTY:
        return APPID_SCEK;
    default:
        return APPID_SCEE;
    }
}

const char *CTitleRegion::GetServiceID() const
{
    static const char SERVICE_ID_SCEK[] = "HP9000-BCAS20058_00";
    static const char SERVICE_ID_SCEE[] = "EP9000-BCES00141_00";
    static const char SERVICE_ID_SCEE_B[] = "EP9000-BCET70011_00";
    static const char SERVICE_ID_SCEA[] = "UP9000-BCUS98148_00";
    static const char SERVICE_ID_SCEA_B[] = "UP9000-NPUA70056_00";
    static const char SERVICE_ID_SCEJ[] = "JP9000-BCJS30018_00";

    switch (Region) {
    case eREGION_SCEE_BETA:
        return SERVICE_ID_SCEE_B;
    case eREGION_SCEA:
    case eREGION_SCEA_GOTY:
        return SERVICE_ID_SCEA;
    case eREGION_SCEA_BETA:
        return SERVICE_ID_SCEA_B;
    case eREGION_SCEJ:
        return SERVICE_ID_SCEJ;
    case eREGION_SCEK:
    case eREGION_SCEK_GOTY:
        return SERVICE_ID_SCEK;
    case eREGION_SCEE:
    case eREGION_SCEE_GOTY:
    default:
        return SERVICE_ID_SCEE;
    }
}

int CTitleRegion::GetRegionSubst() const
{
    switch (Region) {
    case eREGION_SCEA:
    case eREGION_SCEA_GOTY:
        return 0xec1a;
    case eREGION_SCEJ:
        return 0xec18;
    default:
        return 0;
    }
}

bool CTitleRegion::IsGOTYVersion() const
{
    return true;
}

bool CTitleRegion::IsBetaVersion() const
{
    return false;
}

void CTitleRegion::Init(const char *title)
{
    for (int i = 0; i < eREGION_COUNT; ++i) {
        if (strcmp(title, GTitleIDs[i]) == 0) {
            Region = (E_REGION)i;
            DebugRegistry::Register("Change Title ID", (int *)&Region, CycleTitleID);
            return;
        }
    }

    printf("WARNING: Defaulting to SCEE because title ID doesn't match any of our known ones");
    Region = eREGION_SCEE;
    DebugRegistry::Register("Change Title ID", (int *)&Region, CycleTitleID);
}

bool IsKioskDemo()
{
    return false;
}
