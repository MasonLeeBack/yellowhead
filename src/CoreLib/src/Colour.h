#pragma once

#include "types.h"

struct c32 {
    static const c32 White;
    static const c32 Black;
    static const c32 BastardPink;
    static const c32 BabyBlue;
    static const c32 YolkOrange;
    static const c32 TransparentBlack;
    static const c32 DarkGrey;
    static const c32 LightGrey;
    static const c32 Red;
    static const c32 Green;
    static const c32 PastelGreen;

    u32 Bits;

    c32() {}
    c32(u8 r, u8 g, u8 b, u8 a) : Bits((u32(a) << 24) | (u32(r) << 16) | (u32(g) << 8) | u32(b)) {}
    c32(u32 r, u32 g, u32 b, u32 a) : Bits((a << 24) | (r << 16) | (g << 8) | b) {}
    c32(float r, float g, float b, float a);
    explicit c32(u32 bits) : Bits(bits) {}
    explicit c32(v4 colour);

    u8 GetA() const { return Bits >> 24; }
    u8 GetR() const { return Bits >> 16; }
    u8 GetG() const { return Bits >> 8; }
    u8 GetB() const { return Bits; }

    float GetAf() const;
    float GetRf() const;
    float GetGf() const;
    float GetBf() const;
    void GetRGBAf(float& r, float& g, float& b, float& a) const;
    v4 AsV4() const;
    u32 AsGPUCol() const;

    static const c32& FromARGB(u32 bits);
    static const c32& FromGPUCol(u32 bits);
    static u32 Make(u8 r, u8 g, u8 b, u8 a);
    static u32 Make(u32 r, u32 g, u32 b, u32 a);
    static u32 Make(float r, float g, float b, float a);
    static u32 Make(v4 colour);

    bool operator==(const c32& rhs) const { return Bits == rhs.Bits; }
    bool operator!=(const c32& rhs) const { return Bits != rhs.Bits; }
};

u16 Pack565(c32 colour);
c32 Unpack565(u16 colour);
c32 ReplaceA(c32 colour, u8 alpha);
c32 HalfBright(c32 colour);
v4 HSV2RGB(v4 hsv);
v4 RGB2HSV(v4 rgb);
