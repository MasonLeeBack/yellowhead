#pragma once

#include "TextRange.h"
#include "types.h"

template <typename Range>
class CAttributesIterator {
public:
    enum EState {
        STATE_Initial = 0,
        STATE_Running = 1,
        STATE_Done = 2,
    };

    CAttributesIterator(Range range);
    bool Next();
    Range GetKey() const { return Key; }
    Range GetValue() const { return Value; }

private:
    Range RangeValue;
    Range Key;
    Range Value;
    EState State;
};

template <typename Range>
class CTagIterator {
public:
    enum EState {
        STATE_Initial = 0,
        STATE_Running = 1,
        STATE_Done = 2,
    };

    CTagIterator(Range range);
    bool Next();
    Range GetTag() const { return Tag; }
    Range GetAttributes() const { return Attributes; }
    CAttributesIterator<Range> GetAttributesIt() const { return CAttributesIterator<Range>(Attributes); }
    Range GetContents() const { return Contents; }

private:
    Range RangeValue;
    Range Tag;
    Range Attributes;
    Range Contents;
    EState State;
};

template <typename T>
TextRange<T> ExtractTagRaw(TextRange<T>& range);

template <typename T>
TextRange<T> ExtractText(TextRange<T>& range);

template <typename T>
void ExtractKeyValueAndAdvance(TextRange<T>& range, TextRange<T>* key, TextRange<T>* value);

template <typename T>
void ExtractTagNameAndAttributes(TextRange<T> range, TextRange<T>* name, TextRange<T>* attributes);

template <typename T>
void ExtractTagName(TextRange<T> range, TextRange<T>* name);

template <typename T>
bool ExtractTag(TextRange<T>& range, TextRange<T>* tag, TextRange<T>* attributes, TextRange<T>* contents);

template <typename T>
bool ExtractTag(TextRange<T>& range, TextRange<T>* tag, TextRange<T>* contents);

template <typename T>
bool FindNode(TextRange<T> range, const char* tag, TextRange<T>* contents, TextRange<T>* attributes);

template <typename T>
bool FindNode(TextRange<T> range, const char* tag, TextRange<T>* contents);

bool ExtractTagValueFloat(TextRange<char> range, const char* tag, float* value);
bool ExtractTagValueU32(TextRange<char> range, const char* tag, u32* value);
bool ExtractTagValueBool(TextRange<char> range, const char* tag, bool* value);

typedef char check_attributes_iterator_a_size[sizeof(CAttributesIterator<TextRange<char> >) == 0x1c ? 1 : -1];
typedef char check_tag_iterator_a_size[sizeof(CTagIterator<TextRange<char> >) == 0x24 ? 1 : -1];
