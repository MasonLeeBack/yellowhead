#include "types.h"

typedef bool (*SelectLevelHookFn)();
typedef bool (*LevelExitHookFn)();
typedef void (*OnWorldReleaseHookFn)();

struct CAllocatorMM {
};

struct PWorld;

template <typename T, typename Allocator>
struct CRawVector {
    T* Data;
    u32 Size;
    u32 MaxSize;
};

struct CNetworkMessaging {
    u8 Pad[0x1264];
    CRawVector<u32, CAllocatorMM> InputAvailableHistory;
};

struct CNetworkInputManager {
    u8 Pad[0x39c0];

    u32 GetLocalPadsInGameBits() const;
    u32 GetNumPredictedFrames();
};

struct CNetworkGameDataManager {
    u8 UserSelectorForPad[0x20];
    s32 PrepareStage;
    bool RequestChangeLevelInProgress;
    bool LevelDataIsSynced;
    bool HostLevelDataIsSynced;
};

struct CNetworkManager {
    CNetworkMessaging& Messaging;
    CNetworkInputManager& InputManager;
    u8 Pad[0xc];
    CNetworkGameDataManager& GameDataManager;
};

extern CNetworkManager GNetworkManager;

bool IsGamePaused();
bool IsTimeToSwitchLevel();
float GetClockSeconds();
extern "C" int printf(const char* fmt, ...);

float GFrameUpdateTime;
SelectLevelHookFn GSelectLevelHook;
LevelExitHookFn GLevelExitHook;
OnWorldReleaseHookFn GOnWorldReleaseHook;

float GetFrameUpdateTime()
{
    return GFrameUpdateTime;
}

void SetSelectLevelHook(SelectLevelHookFn fn)
{
    GSelectLevelHook = fn;
}

void SetLevelExitHook(LevelExitHookFn fn)
{
    GLevelExitHook = fn;
}

void SetOnWorldReleaseHook(OnWorldReleaseHookFn fn)
{
    GOnWorldReleaseHook = fn;
}

bool AllowedToSendUpdates()
{
    if (IsGamePaused())
        return false;

    bool send_at_reduced_rate = !GNetworkManager.GameDataManager.LevelDataIsSynced;
    send_at_reduced_rate = send_at_reduced_rate | IsTimeToSwitchLevel();
    static float LastReducedRateSend;

    if (!send_at_reduced_rate) {
        LastReducedRateSend = 0.0f;
        return true;
    }

    if (GetClockSeconds() - LastReducedRateSend < 5.0f)
        return false;

    printf("send_at_reduced_rate LastReducedRateSend %.2f => %.2f\n", LastReducedRateSend, GetClockSeconds());
    LastReducedRateSend = GetClockSeconds();

    return true;
}

static bool AllowedToApplySyncedUpdates(u32 num_inputs_applied_this_frame, u32 num_inputs_applied_last_frame)
{
    if (!GNetworkManager.GameDataManager.LevelDataIsSynced)
        return false;

    if (IsTimeToSwitchLevel())
        return false;

    u32 players_to_pack_bits = GNetworkManager.InputManager.GetLocalPadsInGameBits();
    bool have_enough_predicted_inputs_left;

    if (players_to_pack_bits != 0) {
        have_enough_predicted_inputs_left = GNetworkManager.InputManager.GetNumPredictedFrames() != 0;
    } else {
        have_enough_predicted_inputs_left = false;
        const CRawVector<u32, CAllocatorMM>& input_available_history = GNetworkManager.Messaging.InputAvailableHistory;
        if (input_available_history.Size != 0) {
            u32 inputs_available = input_available_history.Data[0];
            have_enough_predicted_inputs_left = inputs_available > 1;
        }
    }

    bool need_to_apply_an_input_to_keep_ticking_over = !have_enough_predicted_inputs_left && (num_inputs_applied_this_frame | num_inputs_applied_last_frame);
    if (need_to_apply_an_input_to_keep_ticking_over)
        return !IsGamePaused();

    return true;
}
