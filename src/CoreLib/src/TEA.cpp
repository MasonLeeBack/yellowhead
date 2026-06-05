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
    (void)force_small_packets;

    u32* words = static_cast<u32*>(data);
    s32 n = size >> 2;
    if ((u32)n <= 1)
        return;

    u32 y = words[0];
    const u32 delta = 0x9e3779b9;
    if (encode_or_decode == EENCODE) {
        u32 z = words[n - 1];
        u32 sum = 0;
        s32 q = 6 + 52 / n;
        if (q == 0)
            return;
        s32 rounds_remaining = q - 1;
        for (;;) {
            sum += delta;
            u32 e = (sum >> 2) & 3;
            s32 p;
            for (p = 0; p < n - 1; ++p) {
                y = words[p + 1];
                z = words[p] += (((z >> 5) ^ (y << 2)) + ((y >> 3) ^ (z << 4))) ^ ((sum ^ y) + (key[(p & 3) ^ e] ^ z));
            }
            y = words[0];
            z = words[n - 1] += (((z >> 5) ^ (y << 2)) + ((y >> 3) ^ (z << 4))) ^ ((sum ^ y) + (key[(p & 3) ^ e] ^ z));
            if (rounds_remaining == 0)
                break;
            --rounds_remaining;
        }
    } else {
        u32 z;
        s32 q = 6 + 52 / n;
        u32 sum = q * delta;
        while (sum != 0) {
            u32 e = (sum >> 2) & 3;
            s32 p;
            for (p = n - 1; p > 0; --p) {
                z = words[p - 1];
                y = words[p] -= (((z >> 5) ^ (y << 2)) + ((y >> 3) ^ (z << 4))) ^ ((sum ^ y) + (key[(p & 3) ^ e] ^ z));
            }
            z = words[n - 1];
            y = words[0] -= (((z >> 5) ^ (y << 2)) + ((y >> 3) ^ (z << 4))) ^ ((sum ^ y) + (key[(p & 3) ^ e] ^ z));
            sum -= delta;
        }
    }
}
