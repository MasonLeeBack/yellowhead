#include "Colour.h"
#include "VectorMathExtras.h"

#include <math.h>

const c32 c32::White(0xffffffff);
const c32 c32::Black(0xff000000);
const c32 c32::BastardPink(0xffe60e63);
const c32 c32::BabyBlue(0xff328adc);
const c32 c32::TransparentBlack(0x00000000);
const c32 c32::YolkOrange(0xffffd303);
const c32 c32::DarkGrey(0xff666666);
const c32 c32::LightGrey(0xffcccccc);
const c32 c32::Red(0xffff0000);
const c32 c32::Green(0xff00ff00);
const c32 c32::PastelGreen(0xff66cc66);

c32 Unpack565(u16 p)
{
    c32 c(u8((p & 0xf800) >> 8), u8((p & 0x07e0) >> 3), u8((p & 0x001f) << 3), u8(0xff));
    return c;
}

c32 ReplaceA(c32 c, u8 a)
{
    c32 r((c.Bits & 0x00ffffff) | (u32(a) << 24));
    return r;
}

c32 HalfBright(c32 c)
{
    c32 r((c.Bits >> 1) & 0x7f7f7f7f);
    return r;
}

static v4 hue(float prop)
{
    float i = prop * 6.0f;
    float d = i - floorf(i);

    switch ((int)i) {
    case 0:
        return v4(d, 0.0f, 1.0f, 0.0f);
    case 1:
        return v4(1.0f, 0.0f, 1.0f - d, 0.0f);
    case 2:
        return v4(1.0f, d, 0.0f, 0.0f);
    case 3:
        return v4(1.0f - d, 1.0f, 0.0f, 0.0f);
    case 4:
        return v4(0.0f, 1.0f, d, 0.0f);
    case 5:
        return v4(0.0f, 1.0f - d, 1.0f, 0.0f);
    case 6:
        return v4(1.0f, 0.0f, 0.0f, 0.0f);
    default:
        return v4(0.0f, 0.0f, 0.0f, 0.0f);
    }
}

u16 Pack565(c32 c)
{
    return ((c.GetR() >> 3) << 11) | ((c.GetG() >> 2) << 5) | (c.GetB() >> 3);
}

v4 HSV2RGB(v4 hsv)
{
    float h = hsv.getX(), s = hsv.getY(), v = hsv.getZ(), a = hsv.getW();

    if (s < 0.0f)
        s = 0.0f;
    else if (s > 1.0f)
        s = 1.0f;

    h = fmodf(h, 1.0f);
    if (h < 0.0f)
        h += 1.0f;

    v4 grey = mulPerElem(v4(v), v4(1.0f, 1.0f, 1.0f, 0.0f));
    grey *= (1.0f - s);
    v4 vv = hue(h);
    vv *= s;
    vv += grey;
    return v4(vv.getX(), vv.getY(), vv.getZ(), a);
}

v4 RGB2HSV(v4 rgb)
{
    static v4 vone(1.0f, 1.0f, 1.0f, 0.0f);

    float r = rgb.getX(), g = rgb.getY(), b = rgb.getZ(), a = rgb.getW();
    float h = 0.0f, s = 0.0f, l;

    v4 normal(g - b, b - r, r - g, 0.0f);
    v4 cols[6] = {
        v4(0.0f, 0.0f, 1.0f, 0.0f),
        v4(1.0f, 0.0f, 1.0f, 0.0f),
        v4(1.0f, 0.0f, 0.0f, 0.0f),
        v4(1.0f, 1.0f, 0.0f, 0.0f),
        v4(0.0f, 1.0f, 0.0f, 0.0f),
        v4(0.0f, 1.0f, 1.0f, 0.0f),
    };
    float dist[6];

    for (int n = 0; n != 6; ++n) {
        dist[n] = dot(cols[n], normal);
    }

    for (int n = 0; n != 6; ++n) {
        int m = (n + 1) % 6;
        if (dist[n] >= 0.0f && dist[m] < 0.0f && dist[n] != dist[m]) {
            h = (n + (dist[n] / (dist[n] - dist[m]))) / 6.0f;
            break;
        }
    }

    v4 hv = hue(h);
    v4 normal2(hv.getY() - hv.getZ(), hv.getZ() - hv.getX(), hv.getX() - hv.getY(), 0.0f);

    l = dot(rgb, normal2);

    if (normal2 == v4(0.0f)) {
        s = 0.0f;
    } else {
        l /= dot(hv, normal2);
        v4 sv1 = hv * l;
        v4 sv2 = vone + ((hv - vone) * l) - sv1;

        float div = dot(vone, sv2);
        if (div != 0.0f)
            s = dot(vone, v4(r, g, b, a) - sv1) / div;
    }

    return v4(h, s, l, a);
}
