#include "filepath.h"

#include "SHA1.h"
#include "StringUtil.h"

#include <ctype.h>

static bool GGameDataReady;
static CFilePath GGameDataPath;
static CFilePath GBaseDir;
static CFilePath GSysCachePath;

namespace
{
    const char* PrependPath(char* dst, const char* filename, const char* path)
    {
        char* result = dst;

        int c = (signed char)filename[0];
        c ^= '/';

        int sign = c >> 31;
        c = (c ^ sign) - sign;

        filename += ((unsigned int)(c - 1) >> 31);

        unsigned int path_len = strlen(path);
        char* append_pos = dst + path_len;

        int needs_slash = 0;
        int slash_offset = 0;

        if (path_len == 0 || path[path_len - 1] != '/')
        {
            needs_slash = 1;
            slash_offset = 1;
        }

        strcpy(append_pos + slash_offset, filename);

        if (path != dst)
        {
            result = dst;
            strncpy(dst, path, path_len);
        }

        if (needs_slash != 0)
        {
            *append_pos = '/';
        }

        return result;
    }
}

CFilePath* GetGameDataPath()
{
    return &GGameDataPath;
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
    temp.Assign(FPR_GAMEDATA, path);
    return FileOpen(temp, handle, OM_Write);
}

void CFilePath::Clear()
{
    Assign("");
}

const char* CFilePath::GetFilename()
{
    const char* result = Filepath;
    
    const char* slash = strrchr(Filepath, '/');
    if (slash != NULL)
        result = slash + 1;

    return result;
}

const char* CFilePath::GetExtension()
{
    char* ext =  strrchr(Filepath, '.');

    if (ext)
        return ext;

    return "";
}

void CFilePath::StripExtension()
{
    char* ext = strrchr(Filepath, '.');

    if (ext)
        *ext = '\0';
}

void CFilePath::StripTrailingSlash()
{
    size_t len = strlen(Filepath);
    size_t last = len - 1;

    if (len == 0)
        return;
    
    char c = Filepath[last];
    if (c == '/' || c == '\\')
    {
        Filepath[last] = '\0';
    }
}

void CFilePath::FixSlashesAndCase()
{
    for (char* p = Filepath; *p != 0; ++p)
    {
        int out = *p;

        if (out == '\\')
        {
            *p = '/';
        }
        else
        {
            if (isupper(out))
            {
                out += 0x20;
            }

            *p = out;
        }
    }
}

void CFilePath::Assign(EFilePathRootDir root_dir, const char* filename)
{
    Invalid = false;

    switch (root_dir)
    {
    case FPR_GAMEDATA:
    {
        CFilePath* game_data_path = GetGameDataPath();
        PrependPath(Filepath, filename, game_data_path->Filepath);
        break;
    }
    case FPR_BLURAY:
        PrependPath(Filepath, filename, GBaseDir.Filepath);
        break;
    case FPR_SYSCACHE:
        PrependPath(Filepath, filename, GSysCachePath.Filepath);
        break;
    default:
        Invalid = true;
        break;
    }
}

void CFilePath::Assign(const char* filepath)
{
    u32 length = StringCopy(Filepath, filepath, 255);
    
    if (length > 254)
    {
        Invalid = true;
        return;
    }

    Invalid = false;
}

void CFilePath::Append(const char* str)
{
    u8 first = str[0];

    if (Filepath[0] != 0)
    {
        u32 len = strlen(Filepath);
        int ends_with_slash = Filepath[len - 1] == '/';

        switch (first)
        {
        default:
            if (ends_with_slash == 0)
            {
                AppendRaw("/");
            }
            break;

        case '/':
            if (ends_with_slash != 0)
            {
                str = str + 1;
            }
            break;
        }
    }

    AppendRaw(str);
}

void CFilePath::AppendRaw(const char* str)
{
    u32 length = StringAppend(Filepath, str, 255);

    if (length > 254)
        Invalid = true;
}
