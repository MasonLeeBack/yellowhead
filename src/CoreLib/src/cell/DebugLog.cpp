#include "types.h"
#include "DebugLog.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/tty.h>

bool GDebugLoggingEnabled;
DebugChannelOptions GDebugChannelMapping[NUM_DEBUG_CHANNELS];

void DebugLogChR(EDebugChannel channel, const char* b, const char* e)
{
    u32 written = e - b;
    if (GDebugChannelMapping[channel].Enabled)
        sys_tty_write(GDebugChannelMapping[channel].TTY, b, e - b, &written);
}

void GetDebugChannelOptions(DebugChannelOptions*& options, u32& count)
{
    options = GDebugChannelMapping;
    count = sizeof(GDebugChannelMapping) / sizeof(GDebugChannelMapping[0]);
}

static void DebugLogChV(EDebugChannel channel, const char* format, va_list args)
{
    char buffer[512];
    if (GDebugChannelMapping[channel].Enabled) {
        vsnprintf(buffer, sizeof(buffer), format, args);
        DebugLogChR(channel, buffer, buffer + strlen(buffer));
    }
}

void DebugLogChF(EDebugChannel channel, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    DebugLogChV(channel, format, args);
    va_end(args);
}

void DebugLog(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    DebugLogChV(DC_DEFAULT, fmt, args);
    va_end(args);
}
