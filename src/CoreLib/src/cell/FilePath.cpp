#include "filepath.h"

static int GLastError;

bool IsLastErrorOutOfDiskSpace()
{
    return false;
}

bool IsLastErrorFileDoesNotExist()
{
    return false;
}

bool DirectoryOpen(const CFilePath& fp, int& handle)
{
    handle = 0;
    return false;
}

bool DirectoryRead(int handle, char* out, u32 out_size)
{
    return false;
}

bool DirectoryClose(int& handle)
{
    handle = 0;
    return true;
}

bool FileCopy(const CFilePath& src, const CFilePath& dst)
{
    return false;
}

bool FileRename(const CFilePath& src, const CFilePath& dst)
{
    return false;
}

bool FileUnlink(const CFilePath& fp)
{
    return false;
}

bool FileStat(const CFilePath& fp, u64& modtime, u64& size)
{
    modtime = 0;
    size = 0;
    return false;
}

bool FileStat(int handle, u64& modtime, u64& size)
{
    modtime = 0;
    size = 0;
    return false;
}

bool FileOpen(const CFilePath& fp, int& handle, EOpenMode mode)
{
    handle = 0;
    return false;
}

bool FileResizeNoZeroFill(const CFilePath& fp, u32 size)
{
    return false;
}

bool FileResize(int handle, u32 size)
{
    return false;
}

bool FileClose(int& handle)
{
    handle = 0;
    return true;
}

u64 FileRead(int handle, void* out, u64 count)
{
    return 0;
}

u64 FileWrite(int handle, const void* data, u64 count)
{
    return 0;
}

u64 FileSeek(int handle, s64 offset, u32 mode)
{
    return 0;
}

bool FileSync(int handle)
{
    return true;
}

bool DirectoryCreate(const char* path)
{
    return false;
}

u32 FileAttributes(const CFilePath& fp)
{
    return 0;
}

u32 FileAttributes(int handle)
{
    return 0;
}

u64 GetFreeDiskSpace(const CFilePath& fp)
{
    return 0;
}
