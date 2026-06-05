#pragma once

#include "MMString.h"
#include "types.h"

enum EURLScheme {
    URL_UNKNOWN = 0,
    URL_HTTP = 1,
    URL_HTTPS = 2,
};

class CHTTPURL {
public:
    EURLScheme scheme;
    MMString<char> host;
    int port;
    MMString<char> path;
    MMString<char> query;
    MMString<char> anchor;
};

class CHTTPClient {
public:
    static void Abort();
};

class CHttpWorkCounter {
public:
    CHttpWorkCounter();
    ~CHttpWorkCounter();
};

void SetDisableContentVerificiation(bool disable);
bool GetDisableContentVerificiation();
u32 GetNumHTTPTaskBaseWorkers();
bool ParseURL(const char* text, CHTTPURL& url);
MMString<char> URLToString(const CHTTPURL& url);

typedef char check_http_url_size[sizeof(CHTTPURL) == 0x50 ? 1 : -1];
typedef char check_http_work_counter_size[sizeof(CHttpWorkCounter) == 0x1 ? 1 : -1];
