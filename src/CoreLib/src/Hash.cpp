#include "Hash.h"

static inline u32 rot(u32 x, u32 k)
{
    return (x << k) | (x >> (32 - k));
}

static inline void mix(u32& a, u32& b, u32& c)
{
    a -= c; a ^= rot(c, 4);  c += b;
    b -= a; b ^= rot(a, 6);  a += c;
    c -= b; c ^= rot(b, 8);  b += a;
    a -= c; a ^= rot(c, 16); c += b;
    b -= a; b ^= rot(a, 19); a += c;
    c -= b; c ^= rot(b, 4);  b += a;
}

static inline void final(u32& a, u32& b, u32& c)
{
    c ^= b; c -= rot(b, 14);
    a ^= c; a -= rot(c, 11);
    b ^= a; b -= rot(a, 25);
    c ^= b; c -= rot(b, 16);
    a ^= c; a -= rot(c, 4);
    b ^= a; b -= rot(a, 14);
    c ^= b; c -= rot(b, 24);
}

u32 CalcAnimationHash(const char* n)
{
    if (!n || *n == 0) {
        return 0;
    }

    u32 hh = 0;
    const char* end = n;
    u32 __h = 0;
    while (*end != 0) {
        if (*end == ' ') {
            if (end[1] == 'L' && end[2] == ' ') {
                end += 3;
                hh = 2;
                continue;
            }
            if (end[1] == 'R' && end[2] == ' ') {
                end += 3;
                hh = 3;
                continue;
            }
        }
        __h = *end + (__h << 6) + (__h << 16) - __h;
        ++end;
    }

    hh += __h << 2;
    if (!hh) {
        ++hh;
    }
    return hh;
}

u32 JenkinsHashU32(const u32* k, u32 length, u32 initial_value)
{
    u32 a, b, c;
    a = b = c = 0xdeadbeef + (length << 2) + initial_value;

    while (length > 3) {
        a += k[0];
        b += k[1];
        c += k[2];
        mix(a, b, c);
        length -= 3;
        k += 3;
    }

    switch (length) {
    case 3:
        c += k[2];
    case 2:
        b += k[1];
    case 1:
        a += k[0];
        final(a, b, c);
    case 0:
        break;
    }

    return c;
}

u32 JenkinsHash(const u8* k, u32 length, u32 initial_value)
{
    u32 a, b, c;
    a = b = 0x9e3779b9;
    c = initial_value;
    u32 len = length;

    while (len >= 12) {
        a += k[0] + (u32(k[1]) << 8) + (u32(k[2]) << 16) + (u32(k[3]) << 24);
        b += k[4] + (u32(k[5]) << 8) + (u32(k[6]) << 16) + (u32(k[7]) << 24);
        c += k[8] + (u32(k[9]) << 8) + (u32(k[10]) << 16) + (u32(k[11]) << 24);
        mix(a, b, c);
        k += 12;
        len -= 12;
    }

    c += length;
    switch (len) {
    default:
        c += u32(k[10]) << 24;
    case 10:
        c += u32(k[9]) << 16;
    case 9:
        c += u32(k[8]) << 8;
    case 8:
        b += u32(k[7]) << 24;
    case 7:
        b += u32(k[6]) << 16;
    case 6:
        b += u32(k[5]) << 8;
    case 5:
        b += k[4];
    case 4:
        a += u32(k[3]) << 24;
    case 3:
        a += u32(k[2]) << 16;
    case 2:
        a += u32(k[1]) << 8;
    case 1:
        a += k[0];
    case 0:
        break;
    }

    mix(a, b, c);
    return c;
}
