#pragma once

#include "SHA1.h"
#include "types.h"

#include <string.h>

class CGUID {
public:
    static const CGUID ZERO;
    u32 ValueU32() const { return Value; }

private:
    u32 Value;
};

class CHash {
public:
    static const CHash ZERO;
    static const CHash EMPTY_STRING;

    CHash() { memset(Bytes, 0, sizeof(Bytes)); }
    CHash(const char* data, u32 size) { SHA1(reinterpret_cast<const u8*>(data), size, Bytes); }

    bool IsSet() const;

private:
    u8 Bytes[20];
};
