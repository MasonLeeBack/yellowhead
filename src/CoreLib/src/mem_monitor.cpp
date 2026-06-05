#include "mem_monitor.h"

#include <string.h>

namespace HeapMon {

static u32 GHeapmonCurrentResourceGUID;

u32 SetCurrentResourceGuid(u32 guid)
{
    u32 old = GHeapmonCurrentResourceGUID;
    GHeapmonCurrentResourceGUID = guid;
    return old;
}

}

CCircularBuffer::CCircularBuffer(u8* buffer, u32 buffer_size)
    : Buffer(buffer), BufferSize(buffer_size), ReadPos(0), WritePos(0), NumBytes(0)
{
}

u32 CCircularBuffer::Read(void* data, u32 size)
{
    if (size > NumBytes)
        size = NumBytes;

    u8* out = (u8*)data;
    u32 remaining = size;

    while (remaining != 0) {
        u32 contiguous = BufferSize - ReadPos;
        if (contiguous > remaining)
            contiguous = remaining;

        if (contiguous != 0) {
            memcpy(out, Buffer + ReadPos, contiguous);
            out += contiguous;
            remaining -= contiguous;
            ReadPos += contiguous;
            NumBytes -= contiguous;
            if (ReadPos >= BufferSize)
                ReadPos = 0;
        }
    }

    return (u32)(out - (u8*)data);
}

bool CCircularBuffer::TryWrite(const void* data, u32 size)
{
    if (!CanAdd(size))
        return false;

    const u8* in = (const u8*)data;
    u32 remaining = size;

    while (remaining != 0) {
        u32 contiguous = BufferSize - WritePos;
        if (contiguous > remaining)
            contiguous = remaining;

        if (contiguous != 0) {
            memcpy(Buffer + WritePos, in, contiguous);
            in += contiguous;
            remaining -= contiguous;
            WritePos += contiguous;
            NumBytes += contiguous;
            if (WritePos >= BufferSize)
                WritePos = 0;
        }
    }

    return true;
}

namespace HeapMon {

void OnNamedEvent(const char* event)
{
    GetHeapMonServer()->OnNamedEvent(event);
}

void OnFlush()
{
    GetHeapMonServer()->OnFlush();
}

void OnSwap()
{
    GetHeapMonServer()->OnSwap();
}

void OnContainerReset(u32 container_id)
{
    GetHeapMonServer()->OnContainerReset(container_id);
}

void OnSetHighwater(u32 container_id, u32 highwater)
{
    GetHeapMonServer()->OnSetHighwater(container_id, highwater);
}

void OnFree(u32 thread_id, const void* ptr)
{
    GetHeapMonServer()->OnFree(thread_id, ptr);
}

void OnReAlloc(u32 thread_id, const void* old_ptr, u32 size, const void* new_ptr)
{
    GetHeapMonServer()->OnReAlloc(thread_id, old_ptr, size, new_ptr, (u32)__builtin_return_address(0));
}

void OnAlloc(u32 thread_id, u32 container_id, const void* ptr)
{
    GetHeapMonServer()->OnAlloc(thread_id, container_id, ptr, (u32)__builtin_return_address(0));
}

void OnReserve(u32 container_id, const void* ptr, const char* name)
{
    GetHeapMonServer()->OnReserve(container_id, ptr, name);
}

void OnSetElfName(const char* elf_name)
{
    GetHeapMonServer()->OnSetElfName(elf_name);
}

void OnNetworkDown()
{
    GetHeapMonServer()->OnNetworkDown();
}

void OnNetworkUp()
{
    GetHeapMonServer()->OnNetworkUp();
}

void OnInitLogging(const char* path)
{
    GetHeapMonServer()->OnInitLogging(path);
}

u32 AllocateContainerID(const void* ptr, u32 size, const char* name)
{
    return GetHeapMonServer()->AllocateContainerID(ptr, size, name);
}

}
