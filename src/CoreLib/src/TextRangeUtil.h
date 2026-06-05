#pragma once

#include "Allocator.h"
#include "GUIDHash.h"
#include "MMString.h"
#include "RawVector.h"
#include "TextRange.h"
#include "types.h"

#include <stddef.h>

void Split(TextRange<char> range, char ch, CRawVector<TextRange<char>, CAllocatorMM>& ranges);

bool TryParseDescriptorString(TextRange<tchar_t> range, char (&out)[41]);
bool TryParseDescriptorString(TextRange<char> range, char (&out)[41]);
bool TryParseStringT(TextRange<char> range, MMString<tchar_t>& out);
bool TryParseBoolValue(TextRange<char> range, bool* out);
bool TryParseHashString(TextRange<char> range, char (&out)[41]);
bool TryParseFloatValue(TextRange<char> range, float* out);
bool TryParseU64Value(TextRange<char> range, u64* out);
bool TryParseU32Value(TextRange<char> range, u32* out);
bool TryParseHashValue(TextRange<char> range, CHash* out);

size_t StringCopy(char* dst, TextRange<char> range, size_t dst_size);

template <int N>
size_t StringCopy(char (&dst)[N], TextRange<char> range);
