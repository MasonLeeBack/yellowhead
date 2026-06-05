#include "ServerSimple.h"

bool CServerSimplePollingWaitingForOML = true;
u32 CServerSimplePollingLastTimeUpdated;

CServerSimple::CServerSimple(u16 port, EBlockingMode blocking_mode) :
    Port(port),
    BlockingMode(blocking_mode),
    Listener(-1)
{
}

CServerSimple::~CServerSimple()
{
}

void CServerSimple::StopListening()
{
    Listener = -1;
}

bool MMSocket::Accept(SOCKET listener, u32 timeout_ms, bool& would_block)
{
    mSocket = listener;
    mBufferCount = 0;
    would_block = false;
    return mSocket >= 0;
}

bool CServerSimple::Accept(MMSocket& socket, u32 timeout_ms, bool& would_block)
{
    if (!IsListening()) {
        would_block = false;
        return false;
    }
    return socket.Accept(Listener, timeout_ms, would_block);
}

bool CServerSimple::StartListening()
{
    Listener = Port;
    return Listener >= 0;
}

bool CServerSimple::IsListening() const
{
    return Listener >= 0;
}
