#pragma once

#include "types.h"

extern u8* GOneFrameScratchPad;
extern u32 GCurScratchPad;
extern u32 ScratchPadSize;

bool ScratchPadInit();
void ScratchPadClean();
void ScratchPadReset();
