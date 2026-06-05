#pragma once

#include <stdint.h>
#include <wchar.h>

#include <vectormath/cpp/vectormath_aos.h>

#if defined(DECOMP_DISABLE_BRANCH_HINTS)
#define likely(x)   (x)
#define unlikely(x) (x)
#else
#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#endif

typedef uint64_t u64;
typedef int64_t  s64;

typedef uint32_t u32;
typedef int32_t  s32;

typedef uint16_t u16;
typedef int16_t  s16;

typedef uint8_t  u8;
typedef int8_t   s8;

typedef Vectormath::Aos::Vector3 v3;
typedef Vectormath::Aos::Vector4 v4;
typedef Vectormath::Aos::Matrix4 m44;
typedef Vectormath::Aos::Matrix3 m33;
typedef Vectormath::Aos::Quat    q4;

typedef u16 tchar_t;

inline const tchar_t* TLiteral(const wchar_t* w)
{
    return reinterpret_cast<const tchar_t*>(w);
}

inline const wchar_t* DebugT2W(const tchar_t* t)
{
    return reinterpret_cast<const wchar_t*>(t);
}

inline const tchar_t* DebugW2T(const wchar_t* w)
{
    return reinterpret_cast<const tchar_t*>(w);
}
