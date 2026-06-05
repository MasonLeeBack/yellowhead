#pragma once

#include "types.h"

extern u8* GOneFrameScratchPad;
extern u32 GCurScratchPad;
extern u32 ScratchPadSize;

void ScratchPadClean();
void ScratchPadReset();
bool ScratchPadInit();

