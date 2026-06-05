#pragma once

#include "types.h"

template <typename T>
class TextRange {
public:
    TextRange() : Begin(0), End(0) {}
    TextRange(const T* begin, const T* end) : Begin(begin), End(end) {}

    int Compare(const T* str) const;
    bool StartsWith(const T* str) const;
    TextRange<T> TrimWhiteQ();
    void TrimWhite();
    bool Find(T ch, TextRange<T>* left) const;
    bool Equals(const TextRange<T>& rhs) const;

    const T* Begin;
    const T* End;
};
