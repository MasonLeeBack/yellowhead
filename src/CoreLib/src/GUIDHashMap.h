#pragma once

#include "Allocator.h"
#include "CalendarTime.h"
#include "GUIDHash.h"
#include "RawVector.h"
#include "filepath.h"
#include "types.h"

class CFileDBRow {
public:
    CFileDBRow();
    ~CFileDBRow();

    void Init(CGUID guid, const char* path);
    void Patch(const CHash& hash);
    void Update(const CHash& hash, u32 size, CalendarTime date);
    CalendarTime GetDate() const { return FileDate; }
    const char* GetFilename() const;
    const char* GetPath() const { return FilePathX ? FilePathX : ""; }
    const CHash& GetHash() const { return FileHash; }
    const CGUID& GetGUID() const { return FileGuid; }
    u32 GetSize() const { return FileSize; }
    void SetPath(const char* path);

public:
    char* FilePathX;
    CalendarTime FileDate;
    CHash FileHash;
    CGUID FileGuid;
    u32 FileSize;
    u32 Padding;
};

struct SComparePath {
    bool operator()(const CFileDBRow& lhs, const CFileDBRow& rhs) const;
    bool operator()(const CFileDBRow& lhs, const CFilePath& rhs) const;
    bool operator()(const CFilePath& lhs, const CFileDBRow& rhs) const;
    bool operator()(const CFilePath& lhs, const CFilePath& rhs) const;
};

struct SCompareGUID {
    bool operator()(const CFileDBRow& lhs, const CFileDBRow& rhs) const;
    bool operator()(const CGUID& lhs, const CFileDBRow& rhs) const;
    bool operator()(const CFileDBRow& lhs, const CGUID& rhs) const;
};

class CFileDB {
public:
    virtual ~CFileDB();
    virtual bool Save();
    virtual bool Patch(const CHash& hash, const CGUID& guid, const CFilePath& path);
    virtual bool ValidateFiles();

    CFileDBRow* FindByGUID(const CGUID& guid);
    CFileDBRow* FindByHash(const CHash& hash);
    CFileDBRow* FindByPath(const CFilePath& path, bool sorted);
    void GetFiles(CVector<CFileDBRow, CAllocatorMM>& files);

protected:
    CFilePath Path;
    CVector<CFileDBRow, CAllocatorMM> Files;
    u32 SortedIndex;
};

class CMutableFileDB : public CFileDB {
public:
    CFileDBRow& AddRow(const CGUID& guid, const char* path);
    bool GetFilename(CGUID guid, CFilePath& path);
    bool GetGUID(const char* path, CGUID& guid);
    CFileDBRow* FindByGUID(const CGUID& guid);
    CFileDBRow* FindByHash(const CHash& hash);
    CFileDBRow* FindByPath(const CFilePath& path, bool sorted);
    void UpdateHash(CFileDBRow& row);
    virtual bool Save();
    virtual bool Patch(const CHash& hash, const CGUID& guid, const CFilePath& path);
};

class CLocalFileDB : public CMutableFileDB {
public:
    bool GetFilename(CGUID guid, CFilePath& path);
    bool GetGUID(const char* path, CGUID& guid);

private:
    bool Searched;
    u8 PaddingLocal[3];
};

class CNetworkFileDB : public CFileDB {
public:
    virtual bool ValidateFiles();
    bool GetFilename(CGUID guid, CFilePath& path);
    bool GetGUID(const char* path, CGUID& guid);
};

void SearchDir(const CFilePath& path, CVector<CFilePath, CAllocatorMM>& files);

typedef char check_file_db_row_size[sizeof(CFileDBRow) == 0x30 ? 1 : -1];
typedef char check_file_db_size[sizeof(CFileDB) == 0x114 ? 1 : -1];
typedef char check_local_file_db_size[sizeof(CLocalFileDB) == 0x118 ? 1 : -1];
typedef char check_network_file_db_size[sizeof(CNetworkFileDB) == 0x114 ? 1 : -1];
