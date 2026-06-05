#include "filepath.h"

#include "SHA1.h"
#include "StringUtil.h"

#include <ctype.h>

static bool GGameDataReady;
static CFilePath GGameDataPath;

CFilePath GetGameDataPath()
{
    if (!GGameDataReady) {
        GGameDataPath.Assign(FPRD_Relative, "gamedata/");
        GGameDataReady = true;
    }
    return GGameDataPath;
}

static void HashBytes(const void* data, u32 size, CHash* hash)
{
    if (!hash)
        return;

    CSHA1Context ctx;
    ctx.Reset();
    ctx.AddData((const u8*)data, size);
    ctx.Result((u8*)hash);
}

bool FileHash(const CFilePath& fp, CHash* hash)
{
    CRawVector<char, CAllocatorMMAligned128> data;
    return FileLoad(fp, data, hash);
}

bool FileSave(const CFilePath& fp, const void* data, u32 size, CHash* hash)
{
    int handle;
    if (!FileOpen(fp, handle, OM_Write))
        return false;

    bool ok = FileWrite(handle, data, size) == size;
    if (ok)
        ok = FileClose(handle);
    else
        FileClose(handle);

    if (ok)
        HashBytes(data, size, hash);
    return ok;
}

bool FileSave(const CFilePath& fp, const CRawVector<char, CAllocatorMMAligned128>& data, CHash* hash)
{
    return FileSave(fp, data.Data, data.Size, hash);
}

bool FileLoad(const CFilePath& fp, CRawVector<char, CAllocatorMMAligned128>& data, CHash* hash)
{
    u64 modtime;
    u64 size64;
    if (!FileStat(fp, modtime, size64))
        return false;

    u32 size = (u32)size64;
    data.try_resize(size);

    int handle;
    if (!FileOpen(fp, handle, OM_Read))
        return false;

    bool ok = FileRead(handle, data.Data, size) == size;
    FileClose(handle);

    if (ok)
        HashBytes(data.Data, size, hash);
    return ok;
}

bool StripAndIgnoreHash(TextRange<char>& line)
{
    while (line.Begin != line.End && isspace((unsigned char)*line.Begin))
        ++line.Begin;
    while (line.Begin != line.End && isspace((unsigned char)line.End[-1]))
        --line.End;

    if (line.Begin == line.End)
        return false;
    if (*line.Begin == '#')
        return false;
    return true;
}

bool LinesLoad(const CRawVector<char, CAllocatorMMAligned128>& data, CVector<MMString<char>, CAllocatorMM>& lines, bool (*filter)(TextRange<char>&))
{
    const char* begin = data.Data;
    const char* end = data.Data + data.Size;

    while (begin != end) {
        const char* cur = begin;
        while (cur != end && *cur != '\r' && *cur != '\n')
            ++cur;

        TextRange<char> line(begin, cur);
        bool keep = filter ? filter(line) : StripAndIgnoreHash(line);
        if (keep) {
            u32 idx = lines.Size;
            lines.try_resize(idx + 1);
            lines[idx].assign(line.Begin, (u32)(line.End - line.Begin));
        }

        begin = cur;
        while (begin != end && (*begin == '\r' || *begin == '\n'))
            ++begin;
    }

    return true;
}

bool FileLoad(const CFilePath& fp, CVector<MMString<char>, CAllocatorMM>& lines, bool (*filter)(TextRange<char>&))
{
    CRawVector<char, CAllocatorMMAligned128> data;
    if (!FileLoad(fp, data, (CHash*)0))
        return false;
    return LinesLoad(data, lines, filter);
}

bool FileOpenTemp(const CFilePath& fp, CFilePath& temp, int& handle)
{
    char path[255];
    FormatString(path, "%s.tmp", fp.c_str());
    temp.Assign(FPRD_Relative, path);
    return FileOpen(temp, handle, OM_Write);
}
