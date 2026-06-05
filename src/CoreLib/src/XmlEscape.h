#pragma once

#include "types.h"

#include <stddef.h>

wchar_t XMLUnescape_(const char*& str, const char* end);
wchar_t XMLUnescape_(const wchar_t*& str, const wchar_t* end);
tchar_t XMLUnescape_(const tchar_t*& str, const tchar_t* end);

wchar_t XMLUnescapeEx(const wchar_t*& str, const wchar_t* end);
tchar_t XMLUnescapeEx(const tchar_t*& str, const tchar_t* end);

u32 UnescapeTChar2TChar_(tchar_t* dst, const tchar_t* src, size_t dst_size);
size_t EscapeXmlEntities_(tchar_t* dst, const tchar_t* src, size_t dst_size);
