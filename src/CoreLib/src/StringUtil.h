#pragma once

#include "MMString.h"
#include "types.h"

#include <stdarg.h>
#include <stddef.h>

size_t StringLength(const char* str);
size_t StringLength(const wchar_t* str);
size_t StringLength(const tchar_t* str);

s32 StringCompare(const char* lhs, const char* rhs);
s32 StringCompare(const wchar_t* lhs, const wchar_t* rhs);
s32 StringCompare(const tchar_t* lhs, const tchar_t* rhs);

s32 StringCompareN(const char* lhs, const char* rhs, size_t count);
s32 StringCompareN(const wchar_t* lhs, const wchar_t* rhs, size_t count);
s32 StringCompareN(const tchar_t* lhs, const tchar_t* rhs, size_t count);

s32 StringICompare(const char* lhs, const char* rhs);
s32 StringICompareN(const char* lhs, const char* rhs, size_t count);

size_t StringCopy(char* dst, const char* src, size_t dst_size);
size_t StringCopy(wchar_t* dst, const wchar_t* src, size_t dst_size);
size_t StringCopy(tchar_t* dst, const tchar_t* src, size_t dst_size);

size_t StringAppend(char* dst, const char* src, size_t dst_size);
size_t StringAppend(wchar_t* dst, const wchar_t* src, size_t dst_size);
size_t StringAppend(tchar_t* dst, const tchar_t* src, size_t dst_size);

const char* StringFind(const char* str, char ch);
const wchar_t* StringFind(const wchar_t* str, wchar_t ch);
const tchar_t* StringFind(const tchar_t* str, tchar_t ch);

size_t FormatString(char* dst, size_t dst_size, const char* fmt, ...);
size_t FormatString(wchar_t* dst, size_t dst_size, const wchar_t* fmt, ...);
size_t FormatString(tchar_t* dst, size_t dst_size, const tchar_t* fmt, ...);

template <u32 N>
u32 FormatString(char (&dst)[N], const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    u32 len = (u32)FormatStringVarArg(dst, N, fmt, args);
    va_end(args);
    return len;
}

size_t FormatStringVarArg(char* dst, size_t dst_size, const char* fmt, va_list args);
size_t FormatStringVarArg(wchar_t* dst, size_t dst_size, const wchar_t* fmt, va_list args);
size_t FormatStringVarArg(tchar_t* dst, size_t dst_size, const tchar_t* fmt, va_list args);

bool CheckHashString(const char* str);
bool CheckFloatString(const char* str);
bool CheckNumberString(const char* str);
bool CheckDescriptorString(const char* str);

class SCompareIgnoreCase {
public:
    bool operator()(const MMString<char>& lhs, const MMString<char>& rhs) const;
};
