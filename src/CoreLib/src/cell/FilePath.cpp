#include "filepath.h"

#include <cell/cell_fs.h>

static __thread CellFsErrno GLastError;

bool IsLastErrorOutOfDiskSpace()
{
    return GLastError == CELL_FS_ERROR_ENOSPC;
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

void DirectoryClose(int& handle)
{
    GLastError = cellFsClosedir(handle);
    handle = -1;
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

void FileClose(int& handle)
{
    if (handle) {
        GLastError = cellFsClose(handle);
    }

    handle = -1;
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
    CellFsStat st;
    
    CellFsErrno ret = cellFsStat(fp.Filepath, &st);
    GLastError = ret;
    
    if (ret == NULL) {
        return 0;
    }

    return st.st_mode;
}

u32 FileAttributes(int handle)
{
    return 0;
}

u64 GetFreeDiskSpace(const CFilePath& fp)
{
    return 0;
}
