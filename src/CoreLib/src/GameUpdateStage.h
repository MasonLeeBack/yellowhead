#pragma once

enum EMainGameUpdateStage {
    E_UPDATE_STAGE_SYNCED = 0,
    E_UPDATE_STAGE_PREDICTED_OR_RENDER = 1,
    E_UPDATE_STAGE_OTHER_WORLD = 2,
    E_UPDATE_STAGE_LOADING = 3,
    E_UPDATE_STAGE_COUNT = 4,
};

class CMainGameStageOverride {
public:
    CMainGameStageOverride(EMainGameUpdateStage new_stage);
    ~CMainGameStageOverride();

private:
    EMainGameUpdateStage PrevMainGameUpdateStage;
};

EMainGameUpdateStage GetMainGameUpdateStage();
void SetMainGameUpdateStage(EMainGameUpdateStage update_stage);
