#include "XmlEscape.h"

template <typename T>
static inline __attribute__((always_inline)) bool MatchEntity(const T*& str, const T* end, const char* entity)
{
    const T* cur = str;
    while (*entity) {
        if (cur == end || *cur != (T)*entity)
            return false;
        ++cur;
        ++entity;
    }
    str = cur;
    return true;
}

template <typename T>
wchar_t XMLUnescape__(const T*& str, const T* end)
{
    if (str == end)
        return 0;

    T ch = *str++;
    if (ch != '&')
        return ch;

    if (MatchEntity(str, end, "amp;"))
        return '&';
    if (MatchEntity(str, end, "lt;"))
        return '<';
    if (MatchEntity(str, end, "gt;"))
        return '>';
    if (MatchEntity(str, end, "quot;"))
        return '"';
    if (MatchEntity(str, end, "apos;"))
        return '\'';
    return '&';
}

tchar_t XMLUnescape_(const tchar_t*& str, const tchar_t* end)
{
    return (tchar_t)XMLUnescape__<tchar_t>(str, end);
}

u32 UnescapeTChar2TChar_(tchar_t* dst, const tchar_t* src, size_t dst_size)
{
    const tchar_t* cur = src;
    const tchar_t* end = src;
    while (*end)
        ++end;

    u32 count = 0;
    while (count + 1 < dst_size && cur != end) {
        dst[count++] = XMLUnescape_(cur, end);
    }
    if (dst_size)
        dst[count] = 0;
    return count;
}

wchar_t XMLUnescape_(const wchar_t*& str, const wchar_t* end)
{
    return XMLUnescape__<wchar_t>(str, end);
}

wchar_t XMLUnescapeEx(const wchar_t*& str, const wchar_t* end)
{
    return XMLUnescape_(str, end);
}

tchar_t XMLUnescapeEx(const tchar_t*& str, const tchar_t* end)
{
    return XMLUnescape_(str, end);
}

wchar_t XMLUnescape_(const char*& str, const char* end)
{
    return XMLUnescape__<char>(str, end);
}

template <typename T>
inline __attribute__((always_inline)) void XMLEscape(T*& dst, wchar_t ch)
{
    const char* entity = 0;
    switch (ch) {
    case '&': entity = "&amp;"; break;
    case '<': entity = "&lt;"; break;
    case '>': entity = "&gt;"; break;
    case '"': entity = "&quot;"; break;
    case '\'': entity = "&apos;"; break;
    default:
        *dst++ = (T)ch;
        return;
    }

    while (*entity)
        *dst++ = (T)*entity++;
}

size_t EscapeXmlEntities_(tchar_t* dst, const tchar_t* src, size_t dst_size)
{
    tchar_t* out = dst;
    tchar_t* end = dst + dst_size;
    while (*src && out + 6 < end)
        XMLEscape(out, *src++);
    if (dst_size)
        *out = 0;
    return out - dst;
}

template wchar_t XMLUnescape__<char>(const char*&, const char*);
template wchar_t XMLUnescape__<wchar_t>(const wchar_t*&, const wchar_t*);
template wchar_t XMLUnescape__<tchar_t>(const tchar_t*&, const tchar_t*);
template void XMLEscape<char>(char*&, wchar_t);
template void XMLEscape<wchar_t>(wchar_t*&, wchar_t);
