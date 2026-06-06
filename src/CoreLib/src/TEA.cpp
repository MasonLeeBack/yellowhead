#include "Tea.h"

void CorrectedBlockTEA(EEncodeOrDecode encode_or_decode, EForceEncodeSmallPackets small_packets, void* data, u32 data_length, const u32 (&key)[4])
{
    (void)small_packets;

    s32 n = data_length >> 2;
    u32* v = static_cast<u32*>(data);
    if ((u32)n <= 1)
        return;

    u32 y = v[0];
    u32 z;
    u32 sum;
    u32 e;
    const u32 DELTA = 0x9e3779b9;
    long p;
    long q;
    if (encode_or_decode == E_TEA_ENCODE) {
        z = v[n - 1];
        sum = 0;
        q = 6 + 52 / n;
        if (q == 0)
            return;
        long rounds_remaining = q - 1;
        for (;;) {
            sum += DELTA;
            e = (sum >> 2) & 3;
            for (p = 0; p < n - 1; ++p) {
                y = v[p + 1];
                z = v[p] += (((z >> 5) ^ (y << 2)) + ((y >> 3) ^ (z << 4))) ^ ((sum ^ y) + (key[(p & 3) ^ e] ^ z));
            }
            y = v[0];
            z = v[n - 1] += (((z >> 5) ^ (y << 2)) + ((y >> 3) ^ (z << 4))) ^ ((sum ^ y) + (key[(p & 3) ^ e] ^ z));
            if (rounds_remaining == 0)
                break;
            --rounds_remaining;
        }
    } else {
        q = 6 + 52 / n;
        sum = q * DELTA;
        while (sum != 0) {
            e = (sum >> 2) & 3;
            for (p = n - 1; p > 0; --p) {
                z = v[p - 1];
                y = v[p] -= (((z >> 5) ^ (y << 2)) + ((y >> 3) ^ (z << 4))) ^ ((sum ^ y) + (key[(p & 3) ^ e] ^ z));
            }
            z = v[n - 1];
            y = v[0] -= (((z >> 5) ^ (y << 2)) + ((y >> 3) ^ (z << 4))) ^ ((sum ^ y) + (key[(p & 3) ^ e] ^ z));
            sum -= DELTA;
        }
    }
}
