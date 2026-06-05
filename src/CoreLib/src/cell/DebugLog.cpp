#include "types.h"

#include <stdarg.h>

enum EDebugChannel {
    DC_None = 0
};

struct DebugChannelOptions {
    u32 Flags;
};

bool GDebugLoggingEnabled;
DebugChannelOptions GDebugChannelMapping[64];

static void DebugLogChV(EDebugChannel channel, const char* prefix, const char* fmt, va_list args)
{
}

void DebugLogChR(EDebugChannel channel, const char* prefix, const char* text)
{
}

DebugChannelOptions* GetDebugChannelOptions(DebugChannelOptions*& options, u32& count)
{
    options = GDebugChannelMapping;
    count = sizeof(GDebugChannelMapping) / sizeof(GDebugChannelMapping[0]);
    return options;
}

void DebugLogChF(EDebugChannel channel, const char* prefix, ...)
{
    va_list args;
    va_start(args, prefix);
    DebugLogChV(channel, prefix, prefix, args);
    va_end(args);
}

void DebugLog(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    DebugLogChV(DC_None, 0, fmt, args);
    va_end(args);
}
