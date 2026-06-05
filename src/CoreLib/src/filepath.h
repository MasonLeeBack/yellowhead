#pragma once

#include "Allocator.h"
#include "GUIDHash.h"
#include "MMString.h"
#include "RawVector.h"
#include "TextRange.h"
#include "types.h"

enum EFilePathRootDir {
    FPRD_Relative = 0
};

enum EOpenMode {
    OM_Read = 0,
    OM_Write = 1
};

class CFilePath {
public:
    CFilePath() : Invalid(false)
    {
        Filepath[0] = 0;
    }

    void Assign(EFilePathRootDir root_dir, const char* path);

    operator const char*() const { return Filepath; }
    const char* c_str() const { return Filepath; }

private:
    char Filepath[255];
    bool Invalid;
};

bool FileStat(const CFilePath& fp, u64& modtime, u64& size);
bool FileStat(int handle, u64& modtime, u64& size);
bool FileUnlink(const CFilePath& fp);
bool FileRename(const CFilePath& src, const CFilePath& dst);
bool FileCopy(const CFilePath& src, const CFilePath& dst);
bool FileOpen(const CFilePath& fp, int& handle, EOpenMode mode);
bool FileClose(int& handle);
bool FileResize(int handle, u32 size);
bool FileResizeNoZeroFill(const CFilePath& fp, u32 size);
u64 FileRead(int handle, void* out, u64 count);
u64 FileWrite(int handle, const void* data, u64 count);
u64 FileSeek(int handle, s64 offset, u32 mode);
bool FileSync(int handle);
u32 FileAttributes(const CFilePath& fp);
u32 FileAttributes(int handle);
u64 GetFreeDiskSpace(const CFilePath& fp);
bool IsLastErrorOutOfDiskSpace();
bool IsLastErrorFileDoesNotExist();

bool DirectoryOpen(const CFilePath& fp, int& handle);
bool DirectoryRead(int handle, char* out, u32 out_size);
bool DirectoryClose(int& handle);
bool DirectoryCreate(const char* path);

CFilePath GetGameDataPath();
bool FileHash(const CFilePath& fp, CHash* hash);
bool FileSave(const CFilePath& fp, const void* data, u32 size, CHash* hash);
bool FileSave(const CFilePath& fp, const CRawVector<char, CAllocatorMMAligned128>& data, CHash* hash);
bool FileLoad(const CFilePath& fp, CRawVector<char, CAllocatorMMAligned128>& data, CHash* hash);
bool StripAndIgnoreHash(TextRange<char>& line);
bool LinesLoad(const CRawVector<char, CAllocatorMMAligned128>& data, CVector<MMString<char>, CAllocatorMM>& lines, bool (*filter)(TextRange<char>&));
bool FileLoad(const CFilePath& fp, CVector<MMString<char>, CAllocatorMM>& lines, bool (*filter)(TextRange<char>&));
bool FileOpenTemp(const CFilePath& fp, CFilePath& temp, int& handle);

inline bool FileExists(const CFilePath& fp)
{
    u64 modtime;
    u64 size;
    return FileStat(fp, modtime, size);
}
