#include "TextRangeUtil.h"

#include "StringUtil.h"

#include <ctype.h>
#include <stdlib.h>

static inline __attribute__((always_inline)) size_t RangeLength(TextRange<char> range)
{
    return range.End - range.Begin;
}

static inline __attribute__((always_inline)) bool RangeEquals(TextRange<char> range, const char* str)
{
    const char* cur = range.Begin;
    while (cur != range.End && *str) {
        if (*cur++ != *str++)
            return false;
    }
    return cur == range.End && *str == 0;
}

static inline __attribute__((always_inline)) bool IsDescriptorChar(char ch)
{
    return isalnum((unsigned char)ch) || ch == '_';
}

void Split(TextRange<char> range, char ch, CRawVector<TextRange<char>, CAllocatorMM>& ranges)
{
    const char* start = range.Begin;
    const char* cur = range.Begin;
    while (cur != range.End) {
        if (*cur == ch) {
            if (ranges.Size == ranges.MaxSize)
                ranges.try_reserve(ranges.Size + 1);
            ranges.Data[ranges.Size++] = TextRange<char>(start, cur);
            start = cur + 1;
        }
        ++cur;
    }

    if (ranges.Size == ranges.MaxSize)
        ranges.try_reserve(ranges.Size + 1);
    ranges.Data[ranges.Size++] = TextRange<char>(start, range.End);
}

bool TryParseDescriptorString(TextRange<tchar_t> range, char (&out)[41])
{
    u32 len = range.End - range.Begin;
    if (len >= 41)
        return false;
    for (u32 i = 0; i < len; ++i) {
        tchar_t ch = range.Begin[i];
        if (i == 0) {
            if (!((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || ch == '_'))
                return false;
        } else if (!((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '_')) {
            return false;
        }
        out[i] = (char)ch;
    }
    out[len] = 0;
    return true;
}

bool TryParseStringT(TextRange<char> range, MMString<tchar_t>& out)
{
    tchar_t tmp[1024];
    u32 len = RangeLength(range);
    if (len >= 1024)
        return false;
    for (u32 i = 0; i < len; ++i)
        tmp[i] = (unsigned char)range.Begin[i];
    tmp[len] = 0;
    out.assign(tmp, len);
    return true;
}

bool TryParseBoolValue(TextRange<char> range, bool* out)
{
    if (RangeEquals(range, "true") || RangeEquals(range, "1")) {
        *out = true;
        return true;
    }
    if (RangeEquals(range, "false") || RangeEquals(range, "0")) {
        *out = false;
        return true;
    }
    return false;
}

size_t StringCopy(char* dst, TextRange<char> range, size_t dst_size)
{
    size_t len = RangeLength(range);
    size_t count = len;
    if (dst_size) {
        if (count >= dst_size)
            count = dst_size - 1;
        for (size_t i = 0; i < count; ++i)
            dst[i] = range.Begin[i];
        dst[count] = 0;
    }
    return len;
}

template <int N>
size_t StringCopy(char (&dst)[N], TextRange<char> range)
{
    return StringCopy(dst, range, N);
}

bool TryParseHashString(TextRange<char> range, char (&out)[41])
{
    if (RangeLength(range) != 40)
        return false;
    for (const char* cur = range.Begin; cur != range.End; ++cur) {
        if (!isxdigit((unsigned char)*cur))
            return false;
    }
    StringCopy(out, range);
    return true;
}

bool TryParseDescriptorString(TextRange<char> range, char (&out)[41])
{
    if (range.Begin == range.End || RangeLength(range) >= 41)
        return false;
    if (!isalpha((unsigned char)*range.Begin) && *range.Begin != '_')
        return false;
    for (const char* cur = range.Begin + 1; cur != range.End; ++cur) {
        if (!IsDescriptorChar(*cur))
            return false;
    }
    StringCopy(out, range);
    return true;
}

bool TryParseFloatValue(TextRange<char> range, float* out)
{
    char tmp[64];
    StringCopy(tmp, range);
    char* end;
    *out = strtof(tmp, &end);
    return end != tmp && *end == 0;
}

bool TryParseU64Value(TextRange<char> range, u64* out)
{
    char tmp[24];
    StringCopy(tmp, range);
    char* end;
    *out = strtoull(tmp, &end, 0);
    return end != tmp && *end == 0;
}

bool TryParseU32Value(TextRange<char> range, u32* out)
{
    u64 value;
    if (!TryParseU64Value(range, &value) || value > 0xffffffffu)
        return false;
    *out = value;
    return true;
}

bool TryParseHashValue(TextRange<char> range, CHash* out)
{
    char tmp[41];
    return TryParseHashString(range, tmp);
}

template size_t StringCopy<41>(char (&)[41], TextRange<char>);
template size_t StringCopy<64>(char (&)[64], TextRange<char>);
template size_t StringCopy<24>(char (&)[24], TextRange<char>);
template size_t StringCopy<16>(char (&)[16], TextRange<char>);
