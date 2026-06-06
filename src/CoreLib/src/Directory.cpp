#include "Directory.h"

#include <string.h>

void DirectoryCreate(const CFilePath& src)
{
    const char* slash = src;

    if (slash == 0 || *slash == 0)
        return;

    while (slash != 0 && *slash != 0) {
        const char* start = slash;
        slash = strchr(start, '\\');
        if (slash == 0) {
            slash = strchr(start, '/');
            if (slash == 0)
                return;
        }

        char temp[1024];
        strcpy(temp, src);

        int offset = slash - (const char*)src;
        temp[offset] = 0;
        DirectoryCreate(temp);

        if (*slash == 0)
            return;

        slash = slash + 1;
    }
}

bool DirectoryExists(const CFilePath& file)
{
    return FileExists(file);
}
