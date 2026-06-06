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

template <typename T>
void ExtractKeyValueAndAdvance(TextRange<T>& range, TextRange<T>* key, TextRange<T>* value)
{
    range = Trim(range);
    const T* cur = range.Begin;
    while (cur != range.End && *cur != '=' && !IsSpaceT(*cur))
        ++cur;
    *key = TextRange<T>(range.Begin, cur);
    while (cur != range.End && (*cur == '=' || IsSpaceT(*cur)))
        ++cur;
    const T* value_begin = cur;
    if (cur != range.End && (*cur == '"' || *cur == '\'')) {
        T quote = *cur++;
        value_begin = cur;
        while (cur != range.End && *cur != quote)
            ++cur;
        *value = TextRange<T>(value_begin, cur);
        if (cur != range.End)
            ++cur;
    } else {
        while (cur != range.End && !IsSpaceT(*cur))
            ++cur;
        *value = TextRange<T>(value_begin, cur);
    }
    range = TextRange<T>(cur, range.End);
}

template <typename T>
void ExtractTagNameAndAttributes(TextRange<T> range, TextRange<T>* name, TextRange<T>* attributes)
{
    range = Trim(range);

    const T* cur = range.Begin;
    while (cur != range.End && !IsSpaceT(*cur))
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
    TextRange<char> raw = ExtractTagRaw(range);
    if (raw.Begin == raw.End)
        return false;
    ExtractTagNameAndAttributes(raw, tag, attributes);

    const char* cur = range.Begin;
    while (cur != range.End && *cur != '<')
        ++cur;
    *contents = TextRange<char>(range.Begin, cur);
    range = TextRange<char>(cur, range.End);
    return true;
}

template <>
bool ExtractTag<tchar_t>(TextRange<tchar_t>& range, TextRange<tchar_t>* tag, TextRange<tchar_t>* contents)
{
    TextRange<tchar_t> attributes;
    TextRange<tchar_t> raw = ExtractTagRaw(range);
    if (raw.Begin == raw.End)
        return false;
    ExtractTagNameAndAttributes(raw, tag, &attributes);
    *contents = ExtractText(range);
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
    if (State == STATE_Done)
        return false;
    ExtractKeyValueAndAdvance(RangeValue, &Key, &Value);
    if (Key.Begin == Key.End) {
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
    if (State == STATE_Done)
        return false;
    if (!ExtractTag(RangeValue, &Tag, &Attributes, &Contents)) {
        State = STATE_Done;
        return false;
    }
    State = STATE_Running;
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
