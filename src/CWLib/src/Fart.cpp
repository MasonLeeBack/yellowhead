#include "types.h"
#include "Allocator.h"

bool UseAccel()
{
    return false;
}

class CCache;

typedef int FileHandle;

u64 FileRead(FileHandle h, void* out, u64 count);
u64 FileSeek(FileHandle h, s64 newpos, u32 whence);
u64 FileWrite(FileHandle h, const void* data, u64 count);
bool FileClose(FileHandle& h);

enum EDebugChannel {
    DC_RESOURCE = 4,
};

void DebugLogChF(EDebugChannel channel, const char* fmt, ...);

inline u64 MIN_MACRO(u64 a, u64 b)
{
    return a < b ? a : b;
}

struct CSHA1Context {
    u8 Bytes[0x64];

    CSHA1Context()
    {
        Reset();
    }

    void Reset();
    void AddData(const u8* data, u32 size);
    int Result(u8* digest);
};

struct CHash {
    u8 Bytes[20];

    CHash()
    {
        Clear();
    }

    void Clear()
    {
        __builtin_memset(Bytes, 0, sizeof(Bytes));
    }

    bool IsSet() const;

    operator bool() const
    {
        return IsSet();
    }

    int Compare(const CHash& rhs) const
    {
        return __builtin_memcmp(Bytes, rhs.Bytes, sizeof(Bytes));
    }

    bool operator==(const CHash& rhs) const
    {
        return Compare(rhs) == 0;
    }

    static char ToHexChar(u8 b)
    {
        char c = b + '0';
        if (b > 9)
            c = b + 'W';
        return c;
    }

    void ConvertToHex(char (&strbuf)[41]) const
    {
        char* p = strbuf;
        for (u32 i = 0; i < sizeof(Bytes); ++i) {
            *p++ = ToHexChar(Bytes[i] >> 4);
            *p++ = ToHexChar(Bytes[i] & 0xf);
        }
        *p = 0;
    }
};

struct CFilePath {
    char Filepath[255];
    bool Invalid;

    CFilePath& operator=(const CFilePath& rhs)
    {
        Assign(rhs.Filepath);
        Invalid = rhs.Invalid;
        return *this;
    }

    void Assign(const char* path);
};

template <typename T, typename Allocator>
struct CRawVector {
    T* Data;
    u32 Size;
    u32 MaxSize;

    bool try_resize(u32 new_size);
};

typedef CRawVector<char, CAllocatorMMAligned128> ByteArray;

struct SResourceReader {
    CCache* Owner;
    FileHandle Handle;
    u32 Offset;
    u32 Size;
    s32 BytesRead;
    u32 OwnerData;
    CSHA1Context RollingHash;
    CHash OriginalHash;

    SResourceReader()
        : Owner(0)
        , Handle(-1)
        , Offset(0)
        , Size(0)
        , BytesRead(0)
        , OwnerData(0)
    {
    }
};

class CCache {
public:
    virtual ~CCache();
    virtual bool IsSlow(const SResourceReader& reader) const;
    virtual bool GetReader(const CHash& hash, SResourceReader& reader);
    virtual void CloseReader(SResourceReader& reader, bool ok);
    virtual bool Unlink(const CHash& hash);
    virtual bool GetSize(const CHash& hash, u32& size);
    virtual bool Put(CHash& out, const void* data, u32 size);
};

class CCacheFS : public CCache {
public:
    CFilePath GetFilename(const CHash& hash);
};

enum CacheType {
    CT_READONLY,
    CT_ACCEL,
    CT_TEMP,
    CT_DOWNLOAD,
    CT_FSB,
    CT_LEVEL,
    CT_LITTLEFARTBUILD,
    CT_SAVEGAME_FIRST,
    CT_SAVEGAME_LAST = 39,
    CT_COUNT,
};

extern CCache* GResourceCaches[];

bool GetFileSizeFromCaches(const CHash& hash, u32& out)
{
    CCache** end = &GResourceCaches[CT_COUNT];
    for (CCache** i = GResourceCaches; i != end; ++i) {
        if (*i && (*i)->GetSize(hash, out))
            return true;
    }

    return false;
}

bool GetResourceReader(const CHash& hash, SResourceReader& out)
{
    CCache** end = &GResourceCaches[CT_COUNT];
    for (CCache** i = GResourceCaches; i != end; ++i) {
        if (*i && (*i)->GetReader(hash, out))
            return true;
    }

    return false;
}

bool SaveFileDataToCache(CacheType type, const void* data, u32 size, CHash& out)
{
    CCache* cache = GResourceCaches[type];
    if (!cache)
        return false;

    return cache->Put(out, data, size);
}

bool SaveFileDataToCache(const void* data, u32 size, CHash& out)
{
    return SaveFileDataToCache(CT_TEMP, data, size, out);
}

bool SaveFileDataToCache(const ByteArray& data, CHash& out)
{
    return SaveFileDataToCache(CT_TEMP, data.Data, data.Size, out);
}

bool SaveFileDataToAccelCache(const ByteArray& data, CHash& out)
{
    return SaveFileDataToCache(CT_ACCEL, data.Data, data.Size, out);
}

bool SaveFileDataToDownloadCache(const ByteArray& data, CHash& out)
{
    return SaveFileDataToCache(CT_DOWNLOAD, data.Data, data.Size, out);
}

CCacheFS* GetFSBCache()
{
    return static_cast<CCacheFS*>(GResourceCaches[CT_FSB]);
}

bool GetFileFromFSBCache(const CHash& hash, CFilePath& path)
{
    CCacheFS* fs = GetFSBCache();

    if (fs) {
        path = fs->GetFilename(hash);
        if (!path.Invalid)
            return path.Filepath[0] != 0;
    }

    return false;
}

u64 FileRead(SResourceReader& h, void* out, u64 count)
{
    u64 rv = FileRead(h.Handle, out, count);

    if (h.BytesRead != -1) {
        h.BytesRead += rv;
        h.RollingHash.AddData(static_cast<const u8*>(out), rv);
    }

    return rv;
}

u64 FileSeek(SResourceReader& h, s64 newpos, u32 whence)
{
    if (whence == 0 && newpos == 0) {
        h.BytesRead = newpos;
        h.RollingHash.Reset();
    } else {
        h.BytesRead = -1;

        if (whence == 1)
            return FileSeek(h.Handle, newpos, 1) - h.Offset;

        if (whence == 2)
            return FileSeek(h.Handle, h.Offset + h.Size + newpos, 0) - h.Offset;
    }

    return FileSeek(h.Handle, h.Offset + newpos, 0) - h.Offset;
}

u64 FileSize(SResourceReader& h)
{
    return h.Size;
}

bool FileClose(SResourceReader& h)
{
    if (h.Handle == -1)
        return true;

    bool rv = true;

    if (h.BytesRead > 0) {
        rv = h.BytesRead == h.Size;
        if (rv && h.OriginalHash) {
            CHash rh;
            h.RollingHash.Result(rh.Bytes);
            if (!(rh == h.OriginalHash)) {
                if (h.Owner) {
                    char original_hash_buf[41], rolling_hash_buf[41];
                    h.OriginalHash.ConvertToHex(original_hash_buf);
                    rh.ConvertToHex(rolling_hash_buf);
                    DebugLogChF(DC_RESOURCE, "Hash mismatch: original: %s, rolling: %s, handle: %d", original_hash_buf, rolling_hash_buf, h.Handle);
                }
                rv = false;
            }
        }
    }

    if (h.Owner)
        h.Owner->CloseReader(h, rv);
    else
        FileClose(h.Handle);

    h.Handle = -1;

    return rv;
}

bool FileLoad(SResourceReader& h, ByteArray& out)
{
    u64 size = FileSize(h);
    bool ok = false;

    if (out.try_resize(size))
        ok = FileRead(h, out.Data, size) == size;

    return FileClose(h) & ok;
}

bool GetFileDataFromCaches(const CHash& hash, ByteArray& out)
{
    SResourceReader reader;
    bool ok = false;

    if (GetResourceReader(hash, reader))
        ok = FileLoad(reader, out);

    return ok;
}

class CQueef {
    enum {
        buffer_size = 32768,
    };

    u8 Buffer[buffer_size];
    SResourceReader Reader;
    bool Used;
    FileHandle Writer;
    u32 Size;

    CQueef();
    ~CQueef();
    bool GetUGCFromCaches(const CHash& hash);
    void OpenForRead();
    bool Copy();
};

CQueef::CQueef()
    : Used(false)
    , Writer(-1)
    , Size(0)
{
    __builtin_memset(Buffer, 0, sizeof(Buffer));
}

bool CQueef::GetUGCFromCaches(const CHash& hash)
{
    if (Used)
        Used = false;

    new (&Reader) SResourceReader;
    Used = true;

    CCache* found = 0;
    CCache** caches = GResourceCaches;
    CCache* cache;

    for (int i = CT_SAVEGAME_FIRST; i < CT_SAVEGAME_LAST; ++i) {
        cache = caches[i];
        if (cache && cache->GetReader(hash, Reader)) {
            found = cache;
            OpenForRead();
            return true;
        }
    }

    cache = caches[CT_DOWNLOAD];
    if (cache && cache->GetReader(hash, Reader)) {
        found = cache;
        OpenForRead();
        return true;
    }

    cache = caches[CT_TEMP];
    if (cache && cache->GetReader(hash, Reader)) {
        found = cache;
        OpenForRead();
        return true;
    }

    cache = caches[CT_LEVEL];
    if (cache && cache->GetReader(hash, Reader)) {
        found = cache;
        OpenForRead();
        return true;
    }

    return false;
}

void CQueef::OpenForRead()
{
    Size = FileSize(Reader);
}

bool CQueef::Copy()
{
    u64 size_left = Size;

    while (size_left != 0) {
        u64 size_to_read = MIN_MACRO(size_left, buffer_size);

        if (FileRead(Reader, Buffer, size_to_read) != size_to_read || FileWrite(Writer, Buffer, size_to_read) != size_to_read) {
            FileClose(Reader);
            return false;
        }

        size_left -= size_to_read;
    }

    bool ok = FileClose(Reader);
    return ok;
}

CQueef::~CQueef()
{
    FileClose(Reader);
}
