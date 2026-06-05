#include "GUIDHashMap.h"

#include "StringUtil.h"

#include <string.h>

bool SCompareGUID::operator()(const CFileDBRow& lhs, const CFileDBRow& rhs) const
{
    return lhs.FileGuid.ValueU32() < rhs.FileGuid.ValueU32();
}

bool SCompareGUID::operator()(const CFileDBRow& lhs, const CGUID& rhs) const
{
    return lhs.FileGuid.ValueU32() < rhs.ValueU32();
}

CFileDBRow::~CFileDBRow()
{
}

void CFileDBRow::Patch(const CHash& hash)
{
    __builtin_memcpy(&FileHash, &hash, sizeof(FileHash));
    __builtin_memcpy(&FileGuid, (const char*)&hash + 16, sizeof(FileGuid));
    FileSize = 0;
    FileDate = 0;
}

void CFileDBRow::Update(const CHash& hash, u32 size, CalendarTime date)
{
    __builtin_memcpy(&FileHash, &hash, sizeof(FileHash));
    __builtin_memcpy(&FileGuid, (const char*)&hash + 16, sizeof(FileGuid));
    FileDate = date;
    FileSize = size;
}

bool CNetworkFileDB::GetFilename(CGUID guid, CFilePath& path)
{
    return false;
}

bool CNetworkFileDB::GetGUID(const char* path, CGUID& guid)
{
    return false;
}

void CFileDBRow::Init(CGUID guid, const char* path)
{
    FileGuid = guid;
    FileSize = 0;
    FileDate = 0;
    SetPath(path);
}

bool SComparePath::operator()(const CFilePath& lhs, const CFilePath& rhs) const
{
    return StringICompare(lhs.c_str(), rhs.c_str()) < 0;
}

bool SComparePath::operator()(const CFileDBRow& lhs, const CFileDBRow& rhs) const
{
    return StringICompare(lhs.GetPath(), rhs.GetPath()) < 0;
}

CFileDBRow::CFileDBRow()
{
    FilePathX = 0;
    FileDate = 0;
    __builtin_memset(&FileHash, 0, sizeof(FileHash));
    __builtin_memset(&FileGuid, 0, sizeof(FileGuid));
    FileSize = 0;
}

const char* CFileDBRow::GetFilename() const
{
    if (!FilePathX)
        return "";

    const char* filename = strrchr(FilePathX, '/');
    if (filename)
        return filename + 1;

    return FilePathX;
}
