#pragma once

#include "types.h"

struct SHA1_CONTEXT {
    u8 Data[100];
};

class CSHA1Context {
public:
    void Reset();
    void AddData(const u8* data, u32 len);
    void Result(u8* digest);

private:
    u8 Data[100];
};

void SHA1(const u8* data, u32 len, u8* digest);
