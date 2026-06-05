#include "GameUpdateStage.h"

EMainGameUpdateStage GMainGameUpdateStage;

EMainGameUpdateStage GetMainGameUpdateStage()
{
    return GMainGameUpdateStage;
}

void SetMainGameUpdateStage(EMainGameUpdateStage update_stage)
{
    GMainGameUpdateStage = update_stage;
}

CMainGameStageOverride::CMainGameStageOverride(EMainGameUpdateStage new_stage)
{
    PrevMainGameUpdateStage = GMainGameUpdateStage;
    GMainGameUpdateStage = new_stage;
}

CMainGameStageOverride::~CMainGameStageOverride()
{
    GMainGameUpdateStage = PrevMainGameUpdateStage;
}
