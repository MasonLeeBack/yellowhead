#include "Directory.h"

#include <string.h>

void DirectoryCreate(const CFilePath& src)
{
    const char* base = src.Filepath;
    const char* scan = base;

    if (scan == 0 || *scan == 0)
    {
        return;
    }

    char temp[1024];
    const char* slash;
    char zero = 0;

    while ((slash = strchr(scan, '\\')) != 0 || (slash = strchr(scan, '/')) != 0)
    {
        strcpy(temp, base);

        s32 offset = slash - base;
        temp[offset] = zero;

        DirectoryCreate((const char*)temp);

        if (*slash == 0)
        {
            return;
        }

        scan = slash + 1;

        if (scan == 0 || *scan == 0)
        {
            return;
        }
    }
}

bool DirectoryExists(const CFilePath& file)
{
    return FileExists(file);
}
