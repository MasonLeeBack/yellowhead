#pragma once

#include "CriticalSec.h"
#include "MMString.h"
#include "STLSet.h"
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

class CHTTPTaskBase;

class CHTTPClient {
public:
    u32 HTTPClientID;
    std::set<CHTTPTaskBase*, std::less<CHTTPTaskBase*>, STLBucketAlloc<CHTTPTaskBase*> > HTTPTransactions;

    static void Abort();
    void Init();
    void Close();
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
typedef char check_http_client_size[sizeof(CHTTPClient) == 0x10 ? 1 : -1];
typedef char check_http_work_counter_size[sizeof(CHttpWorkCounter) == 0x1 ? 1 : -1];
