#include "StringUtil.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

extern "C" int strcasecmp(const char* lhs, const char* rhs);
extern "C" int strncasecmp(const char* lhs, const char* rhs, size_t count);

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
    const T* start = src;
    const T* cur = src;
    T* out = dst;

    if (dst_size != 0) {
        while (--dst_size != 0) {
            T ch = *cur++;
            *out++ = ch;
            if (ch == 0)
                return cur - start - 1;
        }
        *out = 0;
    }

    while (*cur++ != 0) {
    }
    return cur - start - 1;
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
    wchar_t* out = dst;
    const u8* u8str = (const u8*)start;
    const u8* u8str_end = (const u8*)end;

    if (count != 0 && u8str < u8str_end) {
        --count;
        while (count != 0 && u8str < u8str_end) {
            u8 ch = *u8str;
            if ((s8)ch >= 0) {
                ++u8str;
                *out++ = ch;
            } else if (ch <= 0xdf) {
                ++u8str;
                *out = ((ch & 0x1f) << 6) | (u8str[0] & 0x3f);
                if (u8str[0] == 0)
                    break;
                ++u8str;
                ++out;
            } else {
                ++u8str;
                *out = ((ch & 0x0f) << 12) | ((u8str[0] & 0x3f) << 6) | (u8str[1] & 0x3f);
                if (u8str[0] == 0)
                    break;
                ++u8str;
                if (u8str[0] == 0)
                    break;
                ++u8str;
                ++out;
            }
            --count;
        }
    }
    *out = 0;
    if (written)
        *written = out - dst + 1;
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
    char* out = dst;

    if (count != 0 && start < end) {
        --count;
        while (count != 0 && start < end) {
            u32 ch = *start;
            if (ch <= 0x7f) {
                *out++ = ch;
                ++start;
                --count;
            } else if (ch <= 0x7ff) {
                if (count < 2)
                    break;
                *out++ = (ch >> 6) | 0xc0;
                *out++ = (ch & 0x3f) | 0x80;
                ++start;
                count -= 2;
            } else {
                if (count < 3)
                    break;
                *out++ = (ch >> 12) | 0xe0;
                *out++ = ((ch >> 6) & 0x3f) | 0x80;
                *out++ = (ch & 0x3f) | 0x80;
                ++start;
                count -= 3;
            }
        }
    }

    *out = 0;
    if (written)
        *written = out - dst + 1;
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
    while (start < end) {
        char ch = *start++;
        if (ch < 0) {
            if (*start++ == 0)
                break;
            if (((unsigned char)ch & 0xe0) == 0xe0 && *start++ == 0)
                break;
        }
        ++count;
    }
    return count;
}

u32 MultiByteStringLength_Bytes(const wchar_t* start, const wchar_t* end)
{
    u32 count = 0;
    while (start < end) {
        wchar_t ch = *start;
        if (ch <= 0x7f) {
            count += 1;
            ++start;
        } else if (ch <= 0x7ff) {
            ++start;
            count += 2;
        } else {
            count += 3;
            ++start;
        }
    }
    return count;
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
    size_t len = strlen(str);
    for (size_t i = 0; i != len; ++i) {
        char ch = str[i];
        if (!isdigit((unsigned char)ch) && ch != '.' && ch != '-' && ch != '+')
            return false;
    }
    return true;
}

bool CheckNumberString(const char* str)
{
    size_t len = strlen(str);
    for (size_t i = 0; i != len; ++i) {
        if (!isdigit((unsigned char)str[i]))
            return false;
    }
    return true;
}

bool CheckDescriptorString(const char* str)
{
    if (*str != 'g')
        return CheckHashString(str);

    for (u32 i = 1; i != 8;) {
        bool after_first = i > 1;
        char ch = str[i];
        ++i;
        if (ch == 0 && after_first)
            return true;
        if (!isdigit(ch))
            return false;
    }
    return false;
}

size_t StringCopy(char* dst, const char* src, size_t dst_size) { return StringCopy<char>(dst, src, dst_size); }
size_t StringCopy(wchar_t* dst, const wchar_t* src, size_t dst_size) { return StringCopy<wchar_t>(dst, src, dst_size); }
size_t StringCopy(tchar_t* dst, const tchar_t* src, size_t dst_size) { return StringCopy<tchar_t>(dst, src, dst_size); }

size_t StringLength(const char* str) { return strlen(str); }
size_t StringLength(const wchar_t* str) { return wcslen(str); }
size_t StringLength(const tchar_t* str) { return wcslen((const wchar_t*)str); }

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
const tchar_t* StringFind(const tchar_t* str, tchar_t ch) { return (const tchar_t*)wcschr((const wchar_t*)str, ch); }

s32 StringCompareN(const char* lhs, const char* rhs, size_t count) { return strncmp(lhs, rhs, count); }
s32 StringCompareN(const wchar_t* lhs, const wchar_t* rhs, size_t count) { return StringCompareNT(lhs, rhs, count); }
s32 StringCompareN(const tchar_t* lhs, const tchar_t* rhs, size_t count) { return StringCompareNT(lhs, rhs, count); }

s32 StringCompare(const char* lhs, const char* rhs) { return strcmp(lhs, rhs); }
s32 StringCompare(const wchar_t* lhs, const wchar_t* rhs) { return wcscmp(lhs, rhs); }
s32 StringCompare(const tchar_t* lhs, const tchar_t* rhs) { return wcscmp((const wchar_t*)lhs, (const wchar_t*)rhs); }

s32 StringICompareN(const char* lhs, const char* rhs, size_t count)
{
    return strncasecmp(lhs, rhs, count);
}

s32 StringICompare(const char* lhs, const char* rhs)
{
    return strcasecmp(lhs, rhs);
}

bool SCompareIgnoreCase::operator()(const MMString<char>& lhs, const MMString<char>& rhs) const
{
    return StringICompare(lhs.c_str(), rhs.c_str()) < 0;
}

const wchar_t* MultiByteToWChar(MMString<wchar_t>& out, const char* start, const char* end)
{
    if (end == 0)
        end = start + StringLength(start);

    u32 destlen = MultiByteStringLength_Chars(start, end) + 1;
    out.resize(destlen, 0);
    MultiByteToWChar_(out.begin(), start, end, destlen, &destlen);
    if (destlen != 0)
        --destlen;
    out.resize(destlen, 0);
    return out.c_str();
}

const tchar_t* MultiByteToTChar(MMString<tchar_t>& out, const char* start, const char* end)
{
    return (const tchar_t*)MultiByteToWChar((MMString<wchar_t>&)out, start, end);
}

void WCharToMultiByteAppend(MMString<char>& out, const wchar_t* start, const wchar_t* end)
{
    if (end == 0)
        end = start + StringLength(start);

    u32 oldlen = out.size();
    u32 destlen = MultiByteStringLength_Bytes(start, end) + 1;
    out.resize(oldlen + destlen, 0);
    WCharToMultiByte_(out.begin() + oldlen, start, end, destlen, &destlen);
    if (destlen != 0)
        --destlen;
    out.resize(oldlen + destlen, 0);
}

void TCharToMultiByteAppend(MMString<char>& out, const tchar_t* start, const tchar_t* end)
{
    WCharToMultiByteAppend(out, (const wchar_t*)start, (const wchar_t*)end);
}

const char* WCharToMultiByte(MMString<char>& out, const wchar_t* start, const wchar_t* end)
{
    out.resize(0, 0);
    WCharToMultiByteAppend(out, start, end);
    return out.c_str();
}

const char* TCharToMultiByte(MMString<char>& out, const tchar_t* start, const tchar_t* end)
{
    return WCharToMultiByte(out, (const wchar_t*)start, (const wchar_t*)end);
}

template size_t StringCopy<char>(char*, const char*, size_t);
template size_t StringCopy<wchar_t>(wchar_t*, const wchar_t*, size_t);
template size_t StringCopy<tchar_t>(tchar_t*, const tchar_t*, size_t);
template size_t StringAppend<char>(char*, const char*, size_t);
template size_t StringAppend<wchar_t>(wchar_t*, const wchar_t*, size_t);
template size_t StringAppend<tchar_t>(tchar_t*, const tchar_t*, size_t);
