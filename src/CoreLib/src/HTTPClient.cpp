#include "HttpClient.h"

#include "StringUtil.h"

#include <stdlib.h>
#include <string.h>

typedef void* voidpf;

static bool GDisableContentVerificiation;

void SetDisableContentVerificiation(bool disable)
{
    GDisableContentVerificiation = disable;
}

bool GetDisableContentVerificiation()
{
    return GDisableContentVerificiation;
}

extern "C" void comp_free(voidpf opaque, voidpf address)
{
    (void)opaque;
    operator delete(address);
}

extern "C" voidpf comp_alloc(voidpf opaque, unsigned int items, unsigned int size)
{
    (void)opaque;
    unsigned int bytes = items * size;
    void* result = operator new(bytes);
    memset(result, 0, bytes);
    return result;
}

static EURLScheme ParseScheme(const char*& text)
{
    if (strncmp(text, "http://", 7) == 0) {
        text += 7;
        return URL_HTTP;
    }
    if (strncmp(text, "https://", 8) == 0) {
        text += 8;
        return URL_HTTPS;
    }
    return URL_UNKNOWN;
}

bool ParseURL(const char* text, CHTTPURL& url)
{
    if (!text)
        return false;

    url.scheme = ParseScheme(text);
    url.port = url.scheme == URL_HTTPS ? 443 : 80;
    url.host.clear();
    url.path.clear();
    url.query.clear();
    url.anchor.clear();

    const char* host = text;
    text += strcspn(text, ":/?#");
    if (host == text)
        return false;
    url.host.assign(host, text);

    if (*text == ':') {
        ++text;
        char* end;
        unsigned long port = strtoul(text, &end, 10);
        url.port = (int)port;
        if (port - 1 >= 0xfffe)
            return false;
        text = end;
        text += strspn(text, "0123456789");
    }

    if (*text == '/') {
        const char* path = text;
        text += strcspn(text, "?#");
        url.path.assign(path, text);
    } else if (*text == 0 || strchr("?#", *text)) {
        url.path.assign("/");
    } else {
        return false;
    }

    if (*text == '?') {
        const char* query = ++text;
        text += strcspn(text, "#");
        url.query.assign(query, text);
    }

    if (*text == '#') {
        ++text;
        url.anchor.assign(text);
    }

    return !url.host.empty();
}

MMString<char> URLToString(const CHTTPURL& url)
{
    MMString<char> res;
    bool explicit_port = false;

    if (url.scheme == URL_HTTPS) {
        res.assign("https://");
        explicit_port = url.port && url.port != 443;
    } else if (url.scheme == URL_HTTP) {
        res.assign("http://");
        explicit_port = url.port && url.port != 80;
    }

    res.append(url.host);
    if (explicit_port) {
        char buf[16];
        FormatString(buf, ":%d", url.port);
        res.append(buf);
    }

    if (!url.path.empty())
        res.append(url.path);
    else
        res.append("/");

    if (!url.query.empty()) {
        res.append("?");
        res.append(url.query);
    }

    if (!url.anchor.empty()) {
        res.append("#");
        res.append(url.anchor);
    }

    return res;
}

template u32 FormatString<16>(char (&)[16], const char*, ...);
template u32 FormatString<64>(char (&)[64], const char*, ...);
