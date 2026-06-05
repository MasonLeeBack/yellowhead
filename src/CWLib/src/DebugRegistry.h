#pragma once

class DebugRegistry {
public:
    static void Register(const char *name, int *value, void (*callback)());
};
