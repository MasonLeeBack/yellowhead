#pragma once

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
    NUM_DEBUG_CHANNELS = 18,
};

void DebugLogChF(EDebugChannel channel, const char* format, ...);
