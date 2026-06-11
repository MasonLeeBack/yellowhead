#pragma once

#include "filepath.h"
#include "types.h"

class CCircularBuffer {
public:
    CCircularBuffer(u8* buffer, u32 buffer_size);

    bool TryWrite(const void* data, u32 size);
    u32 Read(void* data, u32 size);
    void FlushToDisk(int fd);

    u32 GetNumBytes() const { return NumBytes; }
    bool CanAdd(u32 size) const { return BufferSize - NumBytes >= size; }

private:
    u8* Buffer;
    u32 BufferSize;
    u32 ReadPos;
    u32 WritePos;
    u32 NumBytes;
};

namespace HeapMon {

typedef enum EHeapMonMode {
    HMM_DISABLED = -1,
    HMM_BUFFERING = 0x0,
    HMM_LOG_TO_FILE = 0x1,
    HMM_NETWORK = 0x2
};

class CHeapMonServer {
public:
    bool GetCallstackIndex(u32 count, const u32* entries, u32* index);
    void OnInitLogging(const char* path);
    void OnNetworkDown();
    void OnNetworkUp();
    void OnFlush();
    void OnSwap();
    void OnNamedEvent(const char* event);
    void OnContainerReset(u32 container_id);
    void OnSetHighwater(u32 container_id, u32 highwater);
    void OnReserve(u32 container_id, const void* ptr, const char* name);
    void OnSetElfName(const char* elf_name);
    u32 AllocateContainerID(const void* ptr, u32 size, const char* name);
    void OnFree(u32 thread_id, const void* ptr);
    void OnReAlloc(u32 thread_id, const void* old_ptr, u32 size, const void* new_ptr, u32 caller);
    void OnAlloc(u32 thread_id, u32 container_id, const void* ptr, u32 caller);
};

u32 SetCurrentResourceGuid(u32 guid);
CHeapMonServer* GetHeapMonServer();
void OnNamedEvent(const char* event);
void OnFlush();
void OnSwap();
void OnContainerReset(u32 container_id);
void OnSetHighwater(u32 container_id, u32 highwater);
void OnFree(u32 thread_id, const void* ptr);
void OnReAlloc(u32 thread_id, const void* old_ptr, u32 size, const void* new_ptr);
void OnAlloc(u32 thread_id, u32 container_id, const void* ptr);
void OnReserve(u32 container_id, const void* ptr, const char* name);
void OnSetElfName(const char* elf_name);
void OnNetworkDown();
void OnNetworkUp();
void OnInitLogging(const char* path);
u32 AllocateContainerID(const void* ptr, u32 size, const char* name);

extern EHeapMonMode GHeapMonMode;
extern bool GAllowDroppedEvents;

}

typedef char check_circular_buffer_size[sizeof(CCircularBuffer) == 0x14 ? 1 : -1];
