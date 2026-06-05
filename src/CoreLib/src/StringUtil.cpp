#include "StringUtil.h"

#include <ctype.h>
#include <stdio.h>

template <typename T>
static inline __attribute__((always_inline)) size_t StringLengthT(const T* str)
{
    const T* cur = str;
    while (*cur)
        ++cur;
    return cur - str;
}

template <typename T>
static inline __attribute__((always_inline)) s32 StringCompareT(const T* lhs, const T* rhs)
{
    while (*lhs && *lhs == *rhs) {
        ++lhs;
        ++rhs;
    }
    return (s32)*lhs - (s32)*rhs;
}

template <typename T>
static inline __attribute__((always_inline)) s32 StringCompareNT(const T* lhs, const T* rhs, size_t count)
{
    while (count && *lhs && *lhs == *rhs) {
        --count;
        ++lhs;
        ++rhs;
    }
    if (!count)
        return 0;
    return (s32)*lhs - (s32)*rhs;
}

template <typename T>
size_t StringCopy(T* dst, const T* src, size_t dst_size)
{
    size_t written = 0;
    if (!dst_size)
        return StringLengthT(src);

    while (written + 1 < dst_size && src[written]) {
        dst[written] = src[written];
        ++written;
    }
    dst[written] = 0;
    return written + StringLengthT(src + written);
}

template <typename T>
size_t StringAppend(T* dst, const T* src, size_t dst_size)
{
    size_t len = StringLengthT(dst);
    if (len >= dst_size)
        return len + StringLengthT(src);
    return len + StringCopy(dst + len, src, dst_size - len);
}

template <typename T>
static inline __attribute__((always_inline)) const T* StringFindT(const T* str, T ch)
{
    while (*str) {
        if (*str == ch)
            return str;
        ++str;
    }
    return ch ? 0 : str;
}

const wchar_t* MultiByteToWChar_(wchar_t* dst, const char* start, const char* end, u32 count, u32* written)
{
    u32 n = 0;
    while (n + 1 < count && start != end && *start)
        dst[n++] = (unsigned char)*start++;
    if (count)
        dst[n] = 0;
    if (written)
        *written = n;
    return dst;
}

const tchar_t* MultiByteToTChar_(tchar_t* dst, const char* start, const char* end, u32 count, u32* written)
{
    u32 n = 0;
    while (n + 1 < count && start != end && *start)
        dst[n++] = (unsigned char)*start++;
    if (count)
        dst[n] = 0;
    if (written)
        *written = n;
    return dst;
}

const char* WCharToMultiByte_(char* dst, const wchar_t* start, const wchar_t* end, u32 count, u32* written)
{
    u32 n = 0;
    while (n + 1 < count && start != end && *start)
        dst[n++] = (char)*start++;
    if (count)
        dst[n] = 0;
    if (written)
        *written = n;
    return dst;
}

const char* TCharToMultiByte_(char* dst, const tchar_t* start, const tchar_t* end, u32 count, u32* written)
{
    u32 n = 0;
    while (n + 1 < count && start != end && *start)
        dst[n++] = (char)*start++;
    if (count)
        dst[n] = 0;
    if (written)
        *written = n;
    return dst;
}

u32 MultiByteStringLength_Chars(const char* start, const char* end)
{
    u32 count = 0;
    while (start != end && *start++) {
        ++count;
    }
    return count;
}

u32 MultiByteStringLength_Bytes(const wchar_t* start, const wchar_t* end)
{
    return MultiByteStringLength_Chars((const char*)start, (const char*)end);
}

u32 MultiByteStringLength_Bytes(const tchar_t* start, const tchar_t* end)
{
    return MultiByteStringLength_Chars((const char*)start, (const char*)end);
}

bool CheckHashString(const char* str)
{
    if (*str == '#')
        ++str;
    for (u32 i = 0; i != 40; ++i) {
        if (!isxdigit((unsigned char)str[i]))
            return false;
    }
    return str[40] == 0;
}

bool CheckFloatString(const char* str)
{
    if (*str == '-' || *str == '+')
        ++str;
    bool digit = false;
    while (isdigit((unsigned char)*str)) {
        digit = true;
        ++str;
    }
    if (*str == '.') {
        ++str;
        while (isdigit((unsigned char)*str)) {
            digit = true;
            ++str;
        }
    }
    return digit && *str == 0;
}

bool CheckNumberString(const char* str)
{
    if (*str == '-' || *str == '+')
        ++str;
    if (!isdigit((unsigned char)*str))
        return false;
    while (isdigit((unsigned char)*str))
        ++str;
    return *str == 0;
}

bool CheckDescriptorString(const char* str)
{
    if (!isalpha((unsigned char)*str) && *str != '_')
        return false;
    while (*str) {
        if (!isalnum((unsigned char)*str) && *str != '_')
            return false;
        ++str;
    }
    return true;
}

size_t StringCopy(char* dst, const char* src, size_t dst_size) { return StringCopy<char>(dst, src, dst_size); }
size_t StringCopy(wchar_t* dst, const wchar_t* src, size_t dst_size) { return StringCopy<wchar_t>(dst, src, dst_size); }
size_t StringCopy(tchar_t* dst, const tchar_t* src, size_t dst_size) { return StringCopy<tchar_t>(dst, src, dst_size); }

size_t StringLength(const char* str) { return StringLengthT(str); }
size_t StringLength(const wchar_t* str) { return StringLengthT(str); }
size_t StringLength(const tchar_t* str) { return StringLengthT(str); }

size_t StringAppend(char* dst, const char* src, size_t dst_size) { return StringAppend<char>(dst, src, dst_size); }
size_t StringAppend(wchar_t* dst, const wchar_t* src, size_t dst_size) { return StringAppend<wchar_t>(dst, src, dst_size); }
size_t StringAppend(tchar_t* dst, const tchar_t* src, size_t dst_size) { return StringAppend<tchar_t>(dst, src, dst_size); }

size_t FormatStringVarArg(char* dst, size_t dst_size, const char* fmt, va_list args)
{
    int len = vsnprintf(dst, dst_size, fmt, args);
    return len < 0 ? 0 : (size_t)len;
}

size_t FormatStringVarArg(wchar_t* dst, size_t dst_size, const wchar_t* fmt, va_list args)
{
    int len = vswprintf(dst, dst_size, fmt, args);
    return len < 0 ? 0 : (size_t)len;
}

size_t FormatStringVarArg(tchar_t* dst, size_t dst_size, const tchar_t* fmt, va_list args)
{
    return FormatStringVarArg((wchar_t*)dst, dst_size, (const wchar_t*)fmt, args);
}

size_t FormatString(char* dst, size_t dst_size, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    size_t len = FormatStringVarArg(dst, dst_size, fmt, args);
    va_end(args);
    return len;
}

size_t FormatString(wchar_t* dst, size_t dst_size, const wchar_t* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    size_t len = FormatStringVarArg(dst, dst_size, fmt, args);
    va_end(args);
    return len;
}

size_t FormatString(tchar_t* dst, size_t dst_size, const tchar_t* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    size_t len = FormatStringVarArg(dst, dst_size, fmt, args);
    va_end(args);
    return len;
}

const char* StringFind(const char* str, char ch) { return StringFindT(str, ch); }
const wchar_t* StringFind(const wchar_t* str, wchar_t ch) { return StringFindT(str, ch); }
const tchar_t* StringFind(const tchar_t* str, tchar_t ch) { return StringFindT(str, ch); }

s32 StringCompareN(const char* lhs, const char* rhs, size_t count) { return StringCompareNT(lhs, rhs, count); }
s32 StringCompareN(const wchar_t* lhs, const wchar_t* rhs, size_t count) { return StringCompareNT(lhs, rhs, count); }
s32 StringCompareN(const tchar_t* lhs, const tchar_t* rhs, size_t count) { return StringCompareNT(lhs, rhs, count); }

s32 StringCompare(const char* lhs, const char* rhs) { return StringCompareT(lhs, rhs); }
s32 StringCompare(const wchar_t* lhs, const wchar_t* rhs) { return StringCompareT(lhs, rhs); }
s32 StringCompare(const tchar_t* lhs, const tchar_t* rhs) { return StringCompareT(lhs, rhs); }

s32 StringICompareN(const char* lhs, const char* rhs, size_t count)
{
    while (count && *lhs) {
        s32 diff = tolower((unsigned char)*lhs) - tolower((unsigned char)*rhs);
        if (diff)
            return diff;
        --count;
        ++lhs;
        ++rhs;
    }
    if (!count)
        return 0;
    return tolower((unsigned char)*lhs) - tolower((unsigned char)*rhs);
}

s32 StringICompare(const char* lhs, const char* rhs)
{
    return StringICompareN(lhs, rhs, (size_t)-1);
}

bool SCompareIgnoreCase::operator()(const MMString<char>& lhs, const MMString<char>& rhs) const
{
    return StringICompare(lhs.c_str(), rhs.c_str()) < 0;
}

const wchar_t* MultiByteToWChar(MMString<wchar_t>& out, const char* start, const char* end)
{
    wchar_t tmp[1024];
    u32 written;
    MultiByteToWChar_(tmp, start, end, 1024, &written);
    out.assign(tmp, written);
    return out.c_str();
}

const tchar_t* MultiByteToTChar(MMString<tchar_t>& out, const char* start, const char* end)
{
    tchar_t tmp[1024];
    u32 written;
    MultiByteToTChar_(tmp, start, end, 1024, &written);
    out.assign(tmp, written);
    return out.c_str();
}

void WCharToMultiByteAppend(MMString<char>& out, const wchar_t* start, const wchar_t* end)
{
    char tmp[1024];
    u32 written;
    WCharToMultiByte_(tmp, start, end, 1024, &written);
    out.assign(tmp, written);
}

void TCharToMultiByteAppend(MMString<char>& out, const tchar_t* start, const tchar_t* end)
{
    char tmp[1024];
    u32 written;
    TCharToMultiByte_(tmp, start, end, 1024, &written);
    out.assign(tmp, written);
}

const char* WCharToMultiByte(MMString<char>& out, const wchar_t* start, const wchar_t* end)
{
    WCharToMultiByteAppend(out, start, end);
    return out.c_str();
}

const char* TCharToMultiByte(MMString<char>& out, const tchar_t* start, const tchar_t* end)
{
    TCharToMultiByteAppend(out, start, end);
    return out.c_str();
}

template size_t StringCopy<char>(char*, const char*, size_t);
template size_t StringCopy<wchar_t>(wchar_t*, const wchar_t*, size_t);
template size_t StringCopy<tchar_t>(tchar_t*, const tchar_t*, size_t);
template size_t StringAppend<char>(char*, const char*, size_t);
template size_t StringAppend<wchar_t>(wchar_t*, const wchar_t*, size_t);
template size_t StringAppend<tchar_t>(tchar_t*, const tchar_t*, size_t);
