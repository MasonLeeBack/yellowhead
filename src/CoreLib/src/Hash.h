#pragma once

#include "types.h"

u32 CalcAnimationHash(const char* name);
u32 JenkinsHashU32(const u32* key, u32 length, u32 initval);
u32 JenkinsHash(const u8* key, u32 length, u32 initval);
