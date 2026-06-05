#pragma once

#include "types.h"

static inline u32 MakeBranchDescription(const u16& revision, u32 branch)
{
    return (revision << 16) | branch;
}

static u16 GLeerdammerFormatRevision = 0x0272;
static u32 GLeerdammerBranchDescription = MakeBranchDescription(GLeerdammerFormatRevision, 10);
