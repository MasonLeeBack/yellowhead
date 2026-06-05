#include "Directory.h"

bool DirectoryExists(const CFilePath& file)
{
    return FileExists(file);
}
