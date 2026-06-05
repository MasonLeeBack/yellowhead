#pragma once

#include "types.h"

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

    bool IsSet() const;

private:
    u8 Value[20];
};
