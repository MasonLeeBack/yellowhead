#include "BranchList.h"
#include "SHA1.h"

static inline u32 rol(u32 value, u32 bits)
{
    return (value << bits) | (value >> (32 - bits));
}

void sha1_init(SHA1_CONTEXT* context)
{
    context->h0 = 0x67452301;
    context->count = 0;
    context->h1 = 0xefcdab89;
    context->h2 = 0x98badcfe;
    context->h3 = 0x10325476;
    context->h4 = 0xc3d2e1f0;
    context->nblocks = 0;
}

void transform(SHA1_CONTEXT* context, u8* data)
{
    u32 w[80];
    for (int i = 0; i != 16; ++i) {
        w[i] = (u32(data[0]) << 24) | (u32(data[1]) << 16) | (u32(data[2]) << 8) | u32(data[3]);
        data += 4;
    }

    for (int i = 16; i != 80; ++i)
        w[i] = rol(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);

    u32 a = context->h0;
    u32 b = context->h1;
    u32 c = context->h2;
    u32 d = context->h3;
    u32 e = context->h4;

    for (int i = 0; i != 80; ++i) {
        u32 f;
        u32 k;
        if (i < 20) {
            f = (b & c) | ((~b) & d);
            k = 0x5a827999;
        } else if (i < 40) {
            f = b ^ c ^ d;
            k = 0x6ed9eba1;
        } else if (i < 60) {
            f = (b & c) | (b & d) | (c & d);
            k = 0x8f1bbcdc;
        } else {
            f = b ^ c ^ d;
            k = 0xca62c1d6;
        }

        u32 temp = rol(a, 5) + f + e + k + w[i];
        e = d;
        d = c;
        c = rol(b, 30);
        b = a;
        a = temp;
    }

    context->h0 += a;
    context->h1 += b;
    context->h2 += c;
    context->h3 += d;
    context->h4 += e;
}

void sha1_write(SHA1_CONTEXT* context, u8* data, u32 len)
{
    while (context->count == 64) {
        transform(context, context->buf);
        context->count = 0;
        ++context->nblocks;
    }

    if (data == 0)
        return;

    if (context->count != 0) {
        if (len != 0) {
            while (context->count < 64 && len != 0) {
                context->buf[context->count++] = *data++;
                --len;
            }
            sha1_write(context, 0, 0);
        } else {
            data = 0;
        }

        if (data == 0)
            return;
    }

    while (len > 63) {
        transform(context, data);
        ++context->nblocks;
        context->count = 0;
        data += 64;
        len -= 64;
    }

    if (len == 0)
        return;

    while (context->count < 64 && len != 0) {
        context->buf[context->count++] = *data++;
        --len;
    }
}

void sha1_final(SHA1_CONTEXT* context)
{
    sha1_write(context, 0, 0);

    u32 msb = context->nblocks >> 26;
    u32 lsb = (context->nblocks << 6) + context->count;
    if (lsb < (context->nblocks << 6))
        ++msb;

    msb = (msb << 3) | (lsb >> 29);
    lsb <<= 3;

    int count = context->count;
    context->buf[count++] = 0x80;
    context->count = count;

    if (count > 56) {
        while (count < 64) {
            context->buf[count++] = 0;
            context->count = count;
        }
        sha1_write(context, 0, 0);
        for (count = 0; count != 56; ++count)
            context->buf[count] = 0;
        context->count = 56;
    } else {
        while (count < 56) {
            context->buf[count++] = 0;
            context->count = count;
        }
    }

    context->buf[56] = u8(msb >> 24);
    context->buf[57] = u8(msb >> 16);
    context->buf[58] = u8(msb >> 8);
    context->buf[59] = u8(msb);
    context->buf[60] = u8(lsb >> 24);
    context->buf[61] = u8(lsb >> 16);
    context->buf[62] = u8(lsb >> 8);
    context->buf[63] = u8(lsb);

    transform(context, context->buf);

    u32* digest = (u32*)context->buf;
    digest[0] = context->h0;
    digest[1] = context->h1;
    digest[2] = context->h2;
    digest[3] = context->h3;
    digest[4] = context->h4;
}

ESHA1Result CSHA1Context::Reset()
{
    sha1_init(&Context);
    Corrupted = SHA1_SUCCESS;
    Computed = 0;
    return SHA1_SUCCESS;
}

ESHA1Result CSHA1Context::AddData(const u8* data, u32 len)
{
    if (len == 0)
        return SHA1_SUCCESS;

    if (data == 0)
        return SHA1_NULL;

    if (Computed != 0) {
        Corrupted = SHA1_STATE_ERROR;
        return SHA1_STATE_ERROR;
    }

    if (Corrupted != SHA1_SUCCESS)
        return Corrupted;

    sha1_write(&Context, const_cast<u8*>(data), len);
    return SHA1_SUCCESS;
}

ESHA1Result CSHA1Context::Result(u8* digest)
{
    if (digest == 0)
        return SHA1_NULL;

    if (Corrupted != SHA1_SUCCESS) {
        for (int i = 0; i != 20; ++i)
            digest[i] = 0;
        return Corrupted;
    }

    if (Computed == 0) {
        sha1_final(&Context);
        Computed = 1;
    }

    u8 d0 = Context.buf[0];
    u8 d1 = Context.buf[1];
    u8 d2 = Context.buf[2];
    u8 d3 = Context.buf[3];
    digest[0] = d0;
    digest[1] = d1;
    digest[2] = d2;
    digest[3] = d3;

    d0 = Context.buf[4];
    d1 = Context.buf[5];
    d2 = Context.buf[6];
    d3 = Context.buf[7];
    digest[4] = d0;
    digest[5] = d1;
    digest[6] = d2;
    digest[7] = d3;

    d0 = Context.buf[8];
    d1 = Context.buf[9];
    d2 = Context.buf[10];
    d3 = Context.buf[11];
    digest[8] = d0;
    digest[9] = d1;
    digest[10] = d2;
    digest[11] = d3;

    d0 = Context.buf[12];
    d1 = Context.buf[13];
    d2 = Context.buf[14];
    d3 = Context.buf[15];
    digest[12] = d0;
    digest[13] = d1;
    digest[14] = d2;
    digest[15] = d3;

    d0 = Context.buf[16];
    d1 = Context.buf[17];
    d2 = Context.buf[18];
    d3 = Context.buf[19];
    digest[16] = d0;
    digest[17] = d1;
    digest[18] = d2;
    digest[19] = d3;
    return SHA1_SUCCESS;
}
