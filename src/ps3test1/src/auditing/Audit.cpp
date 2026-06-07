#include "Audit.h"

static CAuditState GAuditState;

CAuditState::CAuditState() :
    LevelVisitor(),
    LevelResults(),
    StartLoadEvent(0),
    FinishLoadEvent(0),
    EndRunEvent(0),
    CurrentFilename(),
    Metrics()
{
}

void CAuditState::Initialise(const char* filename)
{
    u32 random_seed = GIniSettings.GetInt("AuditingRandomSeed", 0);
    u32 start_idx = GIniSettings.GetInt("AuditingStartIdx", 0);
    const char* audit_metrics = GIniSettings.GetString("AuditMetrics", 0);

    if (audit_metrics) {
        TextRange<char> range;
        CRawVector<TextRange<char>, CAllocatorMM> ranges;
        range.Begin = audit_metrics;
        range.End = range.Begin + StringLength(range.Begin);
        Split(range, ',', ranges);

        {
            MMString<char> metric_name;
            CRawVector<CMetric*, CAllocatorMM>* metrics = &Metrics;
            for (u32 i = 0; i < ranges.size(); ++i) {
                const TextRange<char>& metric_range = ranges[i];
                metric_name.assign(metric_range.Begin, metric_range.End - metric_range.Begin);
                const char* metric_name_str = metric_name.c_str();
                CMetric* metric = CAuditMetricFactory::CreateAudit(metric_name_str);
                u32 old_size = metrics->Size;
                if (old_size == metrics->MaxSize) {
                    metrics->try_reserve(old_size + 1);
                    old_size = metrics->Size;
                }
                metrics->Data[old_size] = metric;
                metrics->Size = old_size + 1;
            }
        }
        CAllocatorMM::Free(GAllocatorMM, ranges.Data);
    } else {
        CAuditMetricFactory::CreateAll(Metrics);
    }

    CFilePath path;
    path.Assign(FPR_GAMEDATA, filename);
    DirectoryCreate(path);
    LevelResults.Initialise(path);
    LevelResults.Begin("results", '[');
    LevelVisitor.Initialise(random_seed, start_idx);
}

void CAuditState::Finalise()
{
    LevelResults.End(93);
    LevelResults.Finalise();
}

bool InitAuditing()
{
    const char* filename = GIniSettings.GetString("AuditLog", 0);
    if (filename) {
        GAuditState.Initialise(filename);
        SetSelectLevelHook(SelectNextLevel);
        SetLevelExitHook(PerFrameUpdate);
        SetOnWorldReleaseHook(DisplayReport);
    }
    return true;
}

void FinaliseAuditing()
{
    GAuditState.Finalise();
}
