#pragma once

#include "MMString.h"
#include "types.h"

class MMOTextStreamA {
public:
    struct AsHex {
        u64 Value;
        u32 Width;
    };

    struct FmtInt {
        s64 Value;
        u32 Width;
    };

    virtual ~MMOTextStreamA();

    MMOTextStreamA& operator<<(bool value);
    MMOTextStreamA& operator<<(char value);
    MMOTextStreamA& operator<<(wchar_t value);
    MMOTextStreamA& operator<<(s32 value);
    MMOTextStreamA& operator<<(s64 value);
    MMOTextStreamA& operator<<(u32 value);
    MMOTextStreamA& operator<<(u64 value);
    MMOTextStreamA& operator<<(float value);
    MMOTextStreamA& operator<<(double value);
    MMOTextStreamA& operator<<(AsHex value);
    MMOTextStreamA& operator<<(FmtInt value);
    MMOTextStreamA& operator<<(const char* value);
    MMOTextStreamA& operator<<(const wchar_t* value);
    MMOTextStreamA& operator<<(const tchar_t* value);
    MMOTextStreamA& operator<<(const MMString<char>& value);
    MMOTextStreamA& operator<<(const MMString<wchar_t>& value);
    MMOTextStreamA& operator<<(const MMString<tchar_t>& value);

    virtual void OutputData(const void* data, u32 size) = 0;
    virtual void OutputString(const char* value) = 0;
    virtual void OutputString(const wchar_t* value) = 0;
    virtual void OutputString(const tchar_t* value) = 0;
};

class MMOTextStreamString : public MMOTextStreamA {
public:
    MMOTextStreamString();
    MMOTextStreamString(const MMOTextStreamString& rhs);
    virtual ~MMOTextStreamString();

    virtual void OutputData(const void* data, u32 size);
    virtual void OutputString(const char* value);
    virtual void OutputString(const wchar_t* value);
    virtual void OutputString(const tchar_t* value);

    const char* CStr() const;
    void Clear();

private:
    MMString<char> Result;
};

typedef char check_mmo_text_stream_a_size[sizeof(MMOTextStreamA) == 0x4 ? 1 : -1];
typedef char check_mmo_text_stream_string_size[sizeof(MMOTextStreamString) == 0x18 ? 1 : -1];
