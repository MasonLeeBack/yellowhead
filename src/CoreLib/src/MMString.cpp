#include "MMString.h"

template char* StringTraits<char>::Malloc(u32 count);
template unsigned short* StringTraits<unsigned short>::Malloc(u32 count);
template wchar_t* StringTraits<wchar_t>::Malloc(u32 count);

template bool MMString<char>::Grow(u32 capacity);
template void MMString<char>::clear();
template MMString<char>& MMString<char>::erase(u32 pos, u32 count);
template MMString<char>& MMString<char>::append(const char* start, u32 length);
template MMString<char>& MMString<char>::assign(const char* start, u32 length);
template MMString<char>& MMString<char>::assign(const MMString<char>& rhs);
template MMString<char>& MMString<char>::insert(u32 pos, const char* str, u32 length);
template MMString<char>& MMString<char>::insert(u32 pos, const MMString<char>& rhs, u32 subpos, u32 sublen);
template void MMString<char>::resize(u32 length, char ch);
template MMString<char>& MMString<char>::replace(u32 pos, u32 count, const char* str, u32 length);
template MMString<char>& MMString<char>::replace(u32 pos, u32 count, const MMString<char>& rhs, u32 subpos, u32 sublen);
template void MMString<char>::reserve(u32 capacity);
template void MMString<char>::Construct(const char* start, u32 length);
template void MMString<char>::Terminate(u32 length);
template MMString<char>::size_type MMString<char>::find(const char* str, u32 pos, u32 count) const;
template s32 MMString<char>::compare(const char* rhs) const;
template bool MMString<char>::Contains(const char* str) const;

template bool MMString<unsigned short>::Grow(u32 capacity);
template void MMString<unsigned short>::clear();
template MMString<unsigned short>& MMString<unsigned short>::erase(u32 pos, u32 count);
template MMString<unsigned short>& MMString<unsigned short>::append(const unsigned short* start, u32 length);
template MMString<unsigned short>& MMString<unsigned short>::assign(const unsigned short* start, u32 length);
template MMString<unsigned short>& MMString<unsigned short>::assign(const MMString<unsigned short>& rhs);
template void MMString<unsigned short>::resize(u32 length, unsigned short ch);
template void MMString<unsigned short>::Construct(const unsigned short* start, u32 length);
template void MMString<unsigned short>::Terminate(u32 length);
template MMString<unsigned short> MMString<unsigned short>::substr(u32 pos, u32 count) const;
template s32 MMString<unsigned short>::compare(const unsigned short* rhs) const;

template bool MMString<wchar_t>::Grow(u32 capacity);
template void MMString<wchar_t>::clear();
template MMString<wchar_t>& MMString<wchar_t>::append(const wchar_t* start, u32 length);
template MMString<wchar_t>& MMString<wchar_t>::assign(const wchar_t* start, u32 length);
template MMString<wchar_t>& MMString<wchar_t>::assign(const MMString<wchar_t>& rhs);
template void MMString<wchar_t>::resize(u32 length, wchar_t ch);
template void MMString<wchar_t>::Construct(const wchar_t* start, u32 length);
template void MMString<wchar_t>::Terminate(u32 length);
template MMString<wchar_t>::size_type MMString<wchar_t>::find(const wchar_t* str, u32 pos, u32 count) const;
template MMString<wchar_t> MMString<wchar_t>::substr(u32 pos, u32 count) const;
