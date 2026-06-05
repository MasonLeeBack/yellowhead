#include "Directory.h"

#include <string.h>

void DirectoryCreate(const CFilePath& src)
{
    const char* slash = src;

    if (slash == 0 || *slash == 0)
        return;

    while (slash != 0 && *slash != 0) {
        slash = strchr(slash, '\\');
        if (slash == 0) {
            slash = strchr(slash, '/');
            if (slash == 0)
                return;
        }

        char temp[1024];
        strcpy(temp, src);

        char* next = temp + (slash - (const char*)src);
        *next = 0;
        DirectoryCreate(temp);

        slash = slash + 1;
    }
}

bool DirectoryExists(const CFilePath& file)
{
    return FileExists(file);
}
