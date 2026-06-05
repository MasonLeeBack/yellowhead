#include "types.h"

enum EEncodeOrDecode {
    EENCODE = 0,
    EDECODE = 1,
};

enum EForceEncodeSmallPackets {
    EFESP_No = 0,
    EFESP_Yes = 1,
};

void CorrectedBlockTEA(EEncodeOrDecode encode_or_decode, EForceEncodeSmallPackets force_small_packets, void* data, u32 size, const u32 (&key)[4])
{
}
