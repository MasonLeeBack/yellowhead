#pragma once

#include "Allocator.h"
#include "JobManager.h"
#include "RawVector.h"
#include "RefCount.h"
#include "types.h"

struct SCompressJobResult;
struct SUncompressJobResult;
class CMMSemaphore;

typedef CRawVector<char, CAllocatorMMAligned128> ByteArray;

enum ECompressionJobState {
    CJS_Empty = 0,
    CJS_Queued = 1,
    CJS_Working = 2,
    CJS_Finished = 3,
};

class CompressionJob : public CBaseCounted {
public:
    CompressionJob(CMMSemaphore* done, u32 buffer_size, u32 level);
    virtual ~CompressionJob();

    void EnqueueForCompress(bool high_priority, u32 tag);
    bool IsFinished() const;
    ECompressionJobState GetState() const;
    u32 GetBytesRemaining() const;
    bool Empty() const;
    u16 GetInputBytes() const;
    u16 GetOutputBytes() const;
    void AddData(const void* data, u32 size);
    void Finalise(ByteArray& out);
    static void CompressJob(void* data);

private:
    SCompressJobResult* CompressResult;
    u32 BufferSize;
    void* Buffer;
    void* BufferOut;
    u16 OriginalSize;
    u16 StoredSize;
    CMMSemaphore* Done;
    u32 JobID;
    u32 Level;
    ECompressionJobState State;
    CP<CompressionJob> Prev;
    CP<CompressionJob> Next;
};

class DecompressionJob : public CBaseCounted {
public:
    DecompressionJob(CMMSemaphore* done, const CBaseVector<char>* vector, u32 load_pos, u16 offset, u16 stored_size);
    virtual ~DecompressionJob();

    void EnqueueForDecompress(bool high_priority, u32 tag);
    bool IsFinished() const;
    ECompressionJobState GetState() const;
    u32 GetBytesRemaining() const;
    void GetData(void* out, u32 size);
    u16 GetBufferSize() const;
    bool IsCompressed() const;
    void Finalise();
    static void DecompressJob(void* data);

private:
    SUncompressJobResult* UncompressResult;
    void* Buffer;
    u16 StoredSize;
    u16 OriginalSize;
    const CBaseVector<char>* Vector;
    CMMSemaphore* Done;
    u32 JobID;
    u32 LoadPos;
    u16 Offset;
    ECompressionJobState State;
    CP<DecompressionJob> Next;
};

typedef char check_compression_job_size[sizeof(CompressionJob) == 0x34 ? 1 : -1];
typedef char check_decompression_job_size[sizeof(DecompressionJob) == 0x30 ? 1 : -1];
