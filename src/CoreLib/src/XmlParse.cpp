#include "XmlParse.h"

#include "TextRangeUtil.h"

template <typename T>
static inline __attribute__((always_inline)) bool IsSpaceT(T ch)
{
    return ch <= ' ';
}

template <typename T>
static inline __attribute__((always_inline)) TextRange<T> Trim(TextRange<T> range)
{
    while (range.Begin != range.End && IsSpaceT(*range.Begin))
        ++range.Begin;
    while (range.Begin != range.End && IsSpaceT(range.End[-1]))
        --range.End;
    return range;
}

template <typename T>
static inline __attribute__((always_inline)) void TrimLeft(TextRange<T>& range)
{
    while (range.Begin < range.End && IsSpaceT(*range.Begin))
        ++range.Begin;
}

static inline __attribute__((always_inline)) void TrimLeft(TextRange<char>& range)
{
    while (range.Begin < range.End && (u8)*range.Begin <= ' ')
        ++range.Begin;
}

static inline __attribute__((always_inline)) bool IsAlphaNumericT(char ch)
{
    u8 c = (u8)ch;
    return (u8)(c - 'a') <= 25 || (u8)(c - 'A') <= 25 || (u8)(c - '0') <= 9;
}

template <typename T>
static inline __attribute__((always_inline)) bool IsAlphaNumericT(T ch)
{
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9');
}

template <typename T>
static inline __attribute__((always_inline)) bool EqualsAscii(TextRange<T> range, const char* text)
{
    const T* cur = range.Begin;
    while (cur != range.End && *text) {
        if (*cur++ != (T)*text++)
            return false;
    }
    return cur == range.End && *text == 0;
}

template <typename T>
TextRange<T> ExtractTagRaw(TextRange<T>& range)
{
    const T* begin = range.Begin + 1;
    const T* cur = begin;
    if (range.End > cur) {
        do {
            if (*cur == '>')
                break;
            ++cur;
        } while (range.End > cur);
    }
    const T* next = cur;
    if (next < range.End)
        ++next;
    range = TextRange<T>(next, range.End);
    return TextRange<T>(begin, cur);
}

template <typename T>
TextRange<T> ExtractText(TextRange<T>& range)
{
    const T* cur = range.Begin;
    while (cur != range.End && *cur != '<')
        ++cur;
    TextRange<T> out(range.Begin, cur);
    range = TextRange<T>(cur, range.End);
    return out;
}

template <>
TextRange<tchar_t> ExtractText<tchar_t>(TextRange<tchar_t>& range)
{
    const tchar_t* begin = range.Begin;
    const tchar_t* cur = begin;
    while (cur < range.End && *cur != '<' && *cur != '>' && *cur != 0)
        ++cur;

    TextRange<tchar_t> out(begin, cur);
    if (cur < range.End && *cur != '<')
        ++cur;
    range = TextRange<tchar_t>(cur, range.End);
    return out;
}

template <typename T>
void ExtractKeyValueAndAdvance(TextRange<T>& range, TextRange<T>* key, TextRange<T>* value)
{
    TrimLeft(range);
    const T* key_begin = range.Begin;

    while (range.Begin < range.End && IsAlphaNumericT(*range.Begin))
        ++range.Begin;

    const T* key_end = range.Begin;
    const T* value_begin = range.Begin;
    const T* value_end = range.Begin;

    if (key_end > key_begin && range.Begin < range.End && *range.Begin == '=') {
        ++range.Begin;
        TrimLeft(range);

        value_begin = range.Begin;
        value_end = range.Begin;
        if (range.Begin < range.End && (*range.Begin == '"' || *range.Begin == '\'')) {
            T quote = *range.Begin++;
            value_begin = range.Begin;
            while (range.Begin < range.End && *range.Begin != quote)
                ++range.Begin;
            value_end = range.Begin;
            if (range.Begin < range.End)
                ++range.Begin;
        } else {
            while (range.Begin < range.End && !IsSpaceT(*range.Begin))
                ++range.Begin;
            value_end = range.Begin;
        }
    } else {
        TrimLeft(range);
        value_end = range.Begin;
    }

    TrimLeft(range);
    *key = TextRange<T>(key_begin, key_end);
    *value = TextRange<T>(value_begin, value_end);
}

template <typename T>
void ExtractTagNameAndAttributes(TextRange<T> range, TextRange<T>* name, TextRange<T>* attributes)
{
    TrimLeft(range);

    const T* cur = range.Begin;
    while (cur != range.End && !IsSpaceT(*cur) && *cur != 0)
        ++cur;
    *name = TextRange<T>(range.Begin, cur);
    *attributes = TextRange<T>(cur, range.End).TrimWhiteQ();
}

template <typename T>
void ExtractTagName(TextRange<T> range, TextRange<T>* name)
{
    TextRange<T> attributes;
    ExtractTagNameAndAttributes(range, name, &attributes);
}

template <typename T>
bool ExtractTag(TextRange<T>& range, TextRange<T>* tag, TextRange<T>* attributes, TextRange<T>* contents)
{
    TextRange<T> raw = ExtractTagRaw(range);
    if (raw.Begin == raw.End)
        return false;
    ExtractTagNameAndAttributes(raw, tag, attributes);
    *contents = ExtractText(range);
    return true;
}

template <typename T>
bool ExtractTag(TextRange<T>& range, TextRange<T>* tag, TextRange<T>* contents)
{
    TextRange<T> attributes;
    return ExtractTag(range, tag, &attributes, contents);
}

template <>
bool ExtractTag<char>(TextRange<char>& range, TextRange<char>* tag, TextRange<char>* attributes, TextRange<char>* contents)
{
    if (range.Begin >= range.End || *range.Begin != '<')
        return false;

    TextRange<char> raw = ExtractTagRaw(range);
    ExtractTagNameAndAttributes(raw, tag, attributes);

    const char* contents_begin = range.Begin;
    u32 depth = 0;
    while (range.Begin < range.End) {
        if (*range.Begin != '<') {
            ++range.Begin;
            continue;
        }

        const char* contents_end = range.Begin;
        TextRange<char> raw2 = ExtractTagRaw(range);
        TextRange<char> tag2;
        ExtractTagName(raw2, &tag2);

        bool end_tag = false;
        if (*tag2.Begin == '/') {
            tag2.Begin++;
            end_tag = true;
        }

        if (!tag2.Equals(*tag))
            continue;

        if (end_tag) {
            if (depth == 0) {
                *contents = TextRange<char>(contents_begin, contents_end);
                return true;
            }
            --depth;
        } else {
            ++depth;
        }
    }

    *contents = TextRange<char>(contents_begin, range.End);
    return true;
}

template <>
bool ExtractTag<tchar_t>(TextRange<tchar_t>& range, TextRange<tchar_t>* tag, TextRange<tchar_t>* contents)
{
    if (range.Begin >= range.End || *range.Begin != '<')
        return false;

    TextRange<tchar_t> raw = ExtractTagRaw(range);
    ExtractTagNameAndAttributes(raw, tag, contents);
    return true;
}

template <typename Range>
CAttributesIterator<Range>::CAttributesIterator(Range range) :
    RangeValue(range),
    Key(),
    Value(),
    State(STATE_Initial)
{
}

template <typename Range>
bool CAttributesIterator<Range>::Next()
{
    Range* key = &Key;
    Range* value = &Value;

    if (RangeValue.Begin >= RangeValue.End) {
        State = STATE_Done;
        return false;
    }

    ExtractKeyValueAndAdvance(RangeValue, key, value);
    if (key->Begin >= key->End) {
        State = STATE_Done;
        return false;
    }

    State = STATE_Running;
    return true;
}

template <typename Range>
CTagIterator<Range>::CTagIterator(Range range) :
    RangeValue(range),
    Tag(),
    Attributes(),
    Contents(),
    State(STATE_Initial)
{
}

template <typename Range>
bool CTagIterator<Range>::Next()
{
    State = STATE_Running;
    TrimLeft(RangeValue);

    if (!ExtractTag(RangeValue, &Tag, &Attributes, &Contents)) {
        State = STATE_Done;
        return false;
    }

    TrimLeft(Contents);
    return true;
}

template <typename T>
bool FindNode(TextRange<T> range, const T* tag, TextRange<T>* attributes, TextRange<T>* contents)
{
    TextRange<T> cur = range;
    while (cur.Begin < cur.End) {
        while (cur.Begin < cur.End && *cur.Begin != '<')
            ++cur.Begin;
        if (cur.Begin == cur.End)
            return false;

        TextRange<T> tag_range = cur;
        TextRange<T> raw = ExtractTagRaw(cur);
        TextRange<T> name;
        TextRange<T> attrs;
        ExtractTagNameAndAttributes(raw, &name, &attrs);
        if (name.Compare(tag) == 0)
            return ExtractTag(tag_range, &name, attributes, contents);
    }
    return false;
}

template <typename T>
bool FindNode(TextRange<T> range, const T* tag, TextRange<T>* contents)
{
    TextRange<T> attributes;
    return FindNode(range, tag, &attributes, contents);
}

bool ExtractTagValueFloat(TextRange<char> range, const char* tag, float* value)
{
    TextRange<char> contents;
    if (!FindNode(range, tag, &contents))
        return false;
    return TryParseFloatValue(contents, value);
}

bool ExtractTagValueU32(TextRange<char> range, const char* tag, u32* value)
{
    TextRange<char> contents;
    if (!FindNode(range, tag, &contents))
        return false;
    return TryParseU32Value(contents, value);
}

bool ExtractTagValueBool(TextRange<char> range, const char* tag, bool* value)
{
    TextRange<char> contents;
    if (!FindNode(range, tag, &contents))
        return false;
    return TryParseBoolValue(contents, value);
}

template TextRange<char> ExtractTagRaw<char>(TextRange<char>&);
template TextRange<tchar_t> ExtractTagRaw<tchar_t>(TextRange<tchar_t>&);
template TextRange<tchar_t> ExtractText<tchar_t>(TextRange<tchar_t>&);
template void ExtractKeyValueAndAdvance<char>(TextRange<char>&, TextRange<char>*, TextRange<char>*);
template void ExtractKeyValueAndAdvance<tchar_t>(TextRange<tchar_t>&, TextRange<tchar_t>*, TextRange<tchar_t>*);
template void ExtractTagNameAndAttributes<char>(TextRange<char>, TextRange<char>*, TextRange<char>*);
template void ExtractTagNameAndAttributes<tchar_t>(TextRange<tchar_t>, TextRange<tchar_t>*, TextRange<tchar_t>*);
template void ExtractTagName<char>(TextRange<char>, TextRange<char>*);
template bool ExtractTag<char>(TextRange<char>&, TextRange<char>*, TextRange<char>*, TextRange<char>*);
template bool ExtractTag<tchar_t>(TextRange<tchar_t>&, TextRange<tchar_t>*, TextRange<tchar_t>*);
template bool FindNode<char>(TextRange<char>, const char*, TextRange<char>*, TextRange<char>*);
template bool FindNode<char>(TextRange<char>, const char*, TextRange<char>*);
template CAttributesIterator<TextRange<char> >::CAttributesIterator(TextRange<char>);
template bool CAttributesIterator<TextRange<char> >::Next();
template CTagIterator<TextRange<char> >::CTagIterator(TextRange<char>);
template bool CTagIterator<TextRange<char> >::Next();
