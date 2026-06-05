#pragma once

#include "RawVector.h"
#include "TextRange.h"
#include "thread.h"
#include "types.h"

typedef int SOCKET;

class CAllocatorMMAligned128;

class MMSocket {
public:
    MMSocket();
    ~MMSocket();

    bool IsConnected() const;
    bool WasErrorWouldBlock() const;
    bool Connect(const char* host, u16 port, u32 timeout_ms);
    bool Accept(SOCKET listener, u32 timeout_ms, bool& would_block);
    void Close();
    bool Flush();
    int Recv(char* dst, int size);
    bool Recv(CRawVector<char, CAllocatorMMAligned128>& dst, bool append);
    bool Send(const void* data, u32 size);
    bool SendUnbuffered(const void* data, u32 size);
    bool IsError(int err);
    bool SetBlocking(SOCKET socket, bool blocking);
    s32 GetLastError();

private:
    SOCKET mSocket;
    char mBuffer[1024];
    u32 mBufferCount;
};

class CServerSimple {
public:
    enum EBlockingMode {
        BM_Blocking = 0,
        BM_NonBlocking = 1,
    };

    CServerSimple(u16 port, EBlockingMode blocking_mode);
    ~CServerSimple();

    bool IsListening() const;
    bool Accept(MMSocket& socket, u32 timeout_ms, bool& would_block);
    bool StartListening();
    void StopListening();

private:
    u16 Port;
    EBlockingMode BlockingMode;
    SOCKET Listener;
};

class CServerSimplePolling : public CServerSimple {
public:
    CServerSimplePolling();
    CServerSimplePolling(const CServerSimplePolling& rhs);
    virtual ~CServerSimplePolling();

    void StartThread(const char* name);
    void StopThread();
    bool Poll(u32 timeout_ms);
    virtual void DoWork();
    void ThreadFunc(u64 arg);

private:
    MMSocket Connection;
    THREAD Thread;
    bool Quit;
};

extern bool CServerSimplePollingWaitingForOML;
extern u32 CServerSimplePollingLastTimeUpdated;

typedef char check_mm_socket_size[sizeof(MMSocket) == 0x408 ? 1 : -1];
typedef char check_server_simple_size[sizeof(CServerSimple) == 0x0c ? 1 : -1];
typedef char check_server_simple_polling_size[sizeof(CServerSimplePolling) == 0x428 ? 1 : -1];
