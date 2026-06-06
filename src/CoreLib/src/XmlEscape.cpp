#include "XmlEscape.h"

template <typename T>
static inline __attribute__((always_inline)) int HexDigit(T ch)
{
    if (ch >= (T)'0' && ch <= (T)'9')
        return ch - (T)'0';
    if (ch >= (T)'a' && ch <= (T)'f')
        return ch - (T)'a' + 10;
    if (ch >= (T)'A' && ch <= (T)'F')
        return ch - (T)'A' + 10;
    return -1;
}

template <typename T>
static inline __attribute__((always_inline)) bool TryNumericEntity(const T*& str, const T* end, wchar_t& out)
{
    const T* cur = str;
    u32 value = 0;
    bool hex = false;

    if (cur == end || *cur != (T)'#')
        return false;
    ++cur;

    if (cur != end && (*cur == (T)'x' || *cur == (T)'X')) {
        hex = true;
        ++cur;
    }

    const T* digits = cur;
    while (cur != end && *cur != (T)';') {
        int digit;
        if (hex) {
            digit = HexDigit(*cur);
            if (digit < 0)
                return false;
            value = (value << 4) + digit;
        } else {
            if (*cur < (T)'0' || *cur > (T)'9')
                return false;
            value = value * 10 + (*cur - (T)'0');
        }
        ++cur;
    }

    if (digits == cur || cur == end)
        return false;

    str = cur + 1;
    out = (wchar_t)value;
    return true;
}

template <typename T>
wchar_t XMLUnescape__(const T*& str, const T* end)
{
    if (end && str >= end) {
        str = end;
        return 0;
    }

    const T* cur = str;
    T ch = *cur;
    str = cur + 1;
    if (ch != '&')
        return ch;

    wchar_t numeric;
    if (TryNumericEntity(str, end, numeric))
        return numeric;

    if ((!end || cur + 5 <= end) && cur[1] == (T)'a' && cur[2] == (T)'m' && cur[3] == (T)'p' && cur[4] == (T)';') {
        str = cur + 5;
        return '&';
    }
    if ((!end || cur + 4 <= end) && cur[1] == (T)'l' && cur[2] == (T)'t' && cur[3] == (T)';') {
        str = cur + 4;
        return '<';
    }
    if ((!end || cur + 4 <= end) && cur[1] == (T)'g' && cur[2] == (T)'t' && cur[3] == (T)';') {
        str = cur + 4;
        return '>';
    }
    if ((!end || cur + 6 <= end) && cur[1] == (T)'q' && cur[2] == (T)'u' && cur[3] == (T)'o' && cur[4] == (T)'t' && cur[5] == (T)';') {
        str = cur + 6;
        return '"';
    }
    if ((!end || cur + 6 <= end) && cur[1] == (T)'a' && cur[2] == (T)'p' && cur[3] == (T)'o' && cur[4] == (T)'s' && cur[5] == (T)';') {
        str = cur + 6;
        return '\'';
    }
    if ((!end || cur + 6 <= end) && cur[1] == (T)'n' && cur[2] == (T)'b' && cur[3] == (T)'s' && cur[4] == (T)'p' && cur[5] == (T)';') {
        str = cur + 6;
        return 0xa0;
    }
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
    const wchar_t* cur = str;
    if (*cur == '<' && cur[1] == 'b' && cur[2] == 'r') {
        if (end >= cur + 5 && cur[3] == '/' && cur[4] == '>') {
            str = cur + 5;
            return '\n';
        }
        if (end >= cur + 4 && cur[3] == '>') {
            str = cur + 4;
            return '\n';
        }
    }
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
    case 0xa0: entity = "&nbsp;"; break;
    default:
        *dst++ = (T)ch;
        return;
    }

    while (*entity)
        *dst++ = (T)*entity++;
}

template <>
void XMLEscape<char>(char*& dst, wchar_t ch)
{
    static const char* hex = "0123456789ABCDEF";

    if (ch < 0x20 || ch >= 0x80) {
        *dst++ = '&';
        *dst++ = '#';
        *dst++ = 'x';
        if (ch >= 0x1000)
            *dst++ = hex[(ch >> 12) & 0xf];
        if (ch >= 0x100)
            *dst++ = hex[(ch >> 8) & 0xf];
        if (ch >= 0x10)
            *dst++ = hex[(ch >> 4) & 0xf];
        *dst++ = hex[ch & 0xf];
        *dst++ = ';';
        return;
    }

    if (ch == '\'') {
        *dst++ = '&';
        *dst++ = 'a';
        *dst++ = 'p';
        *dst++ = 'o';
        *dst++ = 's';
        *dst++ = ';';
        return;
    }

    if (ch <= '\'') {
        if (ch == '"') {
            *dst++ = '&';
            *dst++ = 'q';
            *dst++ = 'u';
            *dst++ = 'o';
            *dst++ = 't';
            *dst++ = ';';
            return;
        }

        if (ch == '&') {
            *dst++ = '&';
            *dst++ = 'a';
            *dst++ = 'm';
            *dst++ = 'p';
            *dst++ = ';';
            return;
        }
    } else {
        if (ch == '>') {
            *dst++ = '&';
            *dst++ = 'g';
            *dst++ = 't';
            *dst++ = ';';
            return;
        }

        if (ch == 0xa0) {
            *dst++ = '&';
            *dst++ = 'n';
            *dst++ = 'b';
            *dst++ = 's';
            *dst++ = 'p';
            *dst++ = ';';
            return;
        }

        if (ch == '<') {
            *dst++ = '&';
            *dst++ = 'l';
            *dst++ = 't';
            *dst++ = ';';
            return;
        }
    }

    *dst++ = (char)ch;
    return;
}

static inline __attribute__((always_inline)) size_t EscapedLength(wchar_t ch)
{
    switch (ch) {
    case '&': return 5;
    case '<': return 4;
    case '>': return 4;
    case '"': return 6;
    case '\'': return 6;
    case 0xa0: return 6;
    default: return 1;
    }
}

static inline __attribute__((always_inline)) void XMLEscapeBounded(tchar_t*& dst, tchar_t* end, wchar_t ch)
{
    size_t length = EscapedLength(ch);
    if ((size_t)(end - dst) > length)
        XMLEscape(dst, ch);
}

size_t EscapeXmlEntities_(tchar_t* dst, const tchar_t* src, size_t dst_size)
{
    tchar_t* out = dst;
    tchar_t* end = dst + dst_size;
    size_t count = 0;

    while (*src) {
        wchar_t ch = *src++;
        count += EscapedLength(ch);
        XMLEscapeBounded(out, end, ch);
    }

    if (out < end)
        *out = 0;
    return count;
}

template wchar_t XMLUnescape__<char>(const char*&, const char*);
template wchar_t XMLUnescape__<wchar_t>(const wchar_t*&, const wchar_t*);
template wchar_t XMLUnescape__<tchar_t>(const tchar_t*&, const tchar_t*);
template void XMLEscape<char>(char*&, wchar_t);
template void XMLEscape<wchar_t>(wchar_t*&, wchar_t);
