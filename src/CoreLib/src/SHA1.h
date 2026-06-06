#pragma once

#include "types.h"

enum ESHA1Result {
    SHA1_SUCCESS = 0,
    SHA1_NULL = 1,
    SHA1_INPUT_TOO_LONG = 2,
    SHA1_STATE_ERROR = 3
};

struct SHA1_CONTEXT {
    u32 h0;
    u32 h1;
    u32 h2;
    u32 h3;
    u32 h4;
    u32 nblocks;
    u8 buf[64];
    int count;
};

class CSHA1Context {
public:
    ESHA1Result Reset();
    ESHA1Result AddData(const u8* data, u32 len);
    ESHA1Result Result(u8* digest);

private:
    int Computed;
    ESHA1Result Corrupted;
    SHA1_CONTEXT Context;
};

void SHA1(const u8* data, u32 len, u8* digest);

typedef char check_sha1_context_size[sizeof(SHA1_CONTEXT) == 0x5c ? 1 : -1];
typedef char check_csha1_context_size[sizeof(CSHA1Context) == 0x64 ? 1 : -1];
