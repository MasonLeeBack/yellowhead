#include "types.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/tty.h>

enum EDebugChannel {
    DC_STDOUT = 0,
    DC_DEFAULT = 1,
    DC_SYSTEM = 2,
    DC_RESOURCE = 3,
    DC_PLAYER_PROFILE = 4,
    DC_NETWORK = 5,
    DC_SCRIPT = 6,
    DC_AC = 7,
    DC_PUBLISH = 8,
    DC_HTTP = 9,
    DC_GRAPHICS = 10,
    DC_WEBCAM = 11,
    DC_LOCALISATION = 12,
    DC_INIT = 13,
    DC_BUILD = 14,
    DC_VOIP = 15,
    DC_TESTSUITE = 16,
    DC_REPLAY = 17,
    NUM_DEBUG_CHANNELS = 18
};

struct DebugChannelOptions {
    u32 TTY;
    bool Enabled;
    const char* INI;
};

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
