#include "Serialise.h"

SRevision GLeerdammerFormatRevision;
SRevision GFormatRevision;

ReflectReturn SRevision::CheckBranchDescription() const
{
    if (BranchDescription == 0)
        return REFLECT_OK;

    u32 branch_id = GetBranchID();
    u32 branch_revision = GetBranchRevision();
    u32 current = GFormatRevision.GetBranchDescription();
    u32 current_id = current >> 16;
    u32 current_revision = current & 0xffff;

    if (branch_id != current_id)
        return REFLECT_FORMAT_TOO_NEW;
    if (branch_revision > current_revision)
        return REFLECT_FORMAT_TOO_NEW;
    return REFLECT_OK;
}

void FlattenV2sForSerialisation(float* out, const float* in, u32 count)
{
    for (u32 i = 0; i != count; ++i) {
        out[0] = in[0];
        out[1] = in[1];
        out[2] = in[2];
        in += 4;
        out += 3;
    }
}
