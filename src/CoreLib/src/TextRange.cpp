#include "TextRange.h"

template <>
int TextRange<char>::Compare(const char* str) const
{
    return 0;
}

template <>
bool TextRange<char>::StartsWith(const char* str) const
{
    return false;
}

template <>
TextRange<char> TextRange<char>::TrimWhiteQ()
{
    return *this;
}

template <>
void TextRange<char>::TrimWhite()
{
}

template <>
bool TextRange<char>::Find(char ch, TextRange<char>* left) const
{
    return false;
}

template <>
bool TextRange<char>::Equals(const TextRange<char>& rhs) const
{
    return false;
}

template <>
int TextRange<tchar_t>::Compare(const tchar_t* str) const
{
    return 0;
}

template <>
TextRange<tchar_t> TextRange<tchar_t>::TrimWhiteQ()
{
    return *this;
}

template <>
void TextRange<tchar_t>::TrimWhite()
{
}
