#pragma once

#include "types.h"

enum EEncodeOrDecode {
    E_TEA_ENCODE = 0,
    E_TEA_DECODE = 1
};

enum EForceEncodeSmallPackets {
    E_FORCE_ENCODE_SMALL_PACKETS = 0,
    E_DONT_ENCODE_SMALL_PACKETS = 1
};

static const u32 KEY_LENGTH = 4;
extern const u32 DEFAULT_TEA_KEY[KEY_LENGTH];

void CorrectedBlockTEA(EEncodeOrDecode encode_or_decode, EForceEncodeSmallPackets small_packets, void* data, u32 data_length, const u32 (&key)[4]);
