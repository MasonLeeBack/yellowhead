#include "CompressionJob.h"
#include "Fifo.h"

#include <string.h>

struct SCompressJobResult {
    s32 Result;
    u32 StoredSize;
};

struct SUncompressJobResult {
    s32 Result;
};

extern "C" int compress2(void* dest, u32* dest_len, const void* source, u32 source_len, int level);
extern "C" int uncompress(void* dest, u32* dest_len, const void* source, u32 source_len);

void CompressOnSPU(void* dest, u32 dest_len, const void* source, u32 source_len, SCompressJobResult* result, u32 tag);
void UncompressOnSPU(void* dest, u32 dest_len, const void* source, u32 source_len, SUncompressJobResult* result);

CompressionJob::CompressionJob(CMMSemaphore* done, u32 buffer_size, u32 level) :
    CompressResult(static_cast<SCompressJobResult*>(GOtherBucket.AlignedMalloc(0x80, 0x80))),
    BufferSize(buffer_size),
    Buffer(GOtherBucket.AlignedMalloc(buffer_size, 0x80)),
    BufferOut(0),
    OriginalSize(0),
    StoredSize(0),
    Done(done),
    JobID(0),
    Level(level),
    State(CJS_BEGIN),
    Prev(),
    Next()
{
    CompressResult->Result = 0;
    CompressResult->StoredSize = 0;
}

CompressionJob::~CompressionJob()
{
    GOtherBucket.AlignedFree(CompressResult);
    GOtherBucket.AlignedFree(Buffer);
    GOtherBucket.AlignedFree(BufferOut);
}

bool CompressionJob::IsFinished() const
{
    if (State != CJS_WORKING)
        return State == CJS_DONE;

    if (CompressResult->Result == 0)
        return false;

    if (JobID == 0)
        return true;

    return GJobManager->CountJobsWithJobID(JobID) == 0;
}

void CompressionJob::Finalise(ByteArray& out)
{
    StoredSize = CompressResult->StoredSize;

    u32 size = StoredSize;
    void* data = BufferOut;
    if (size == 0 || OriginalSize <= size) {
        size = OriginalSize;
        data = Buffer;
        StoredSize = OriginalSize;
    }

    u32 old_size = out.Size;
    out.try_resize(old_size + size);
    memcpy(out.Data + old_size, data, StoredSize);

    GOtherBucket.AlignedFree(BufferOut);
    GOtherBucket.AlignedFree(Buffer);
    Buffer = 0;
    State = CJS_DONE;
    BufferOut = 0;
}

void CompressionJob::EnqueueForCompress(bool high_priority, u32 tag)
{
    BufferOut = GOtherBucket.AlignedMalloc(OriginalSize, 0x80);
    CompressResult->Result = 0;
    CompressResult->StoredSize = 0;
    State = CJS_WORKING;
    if (high_priority) {
        CompressOnSPU(BufferOut, OriginalSize, Buffer, OriginalSize, CompressResult, tag);
    } else {
        JobID = GJobManager->EnqueueJob(1000, CompressionJob::CompressJob, this, tag, "CompressionJob");
    }
}

void CompressionJob::CompressJob(void* data)
{
    CompressionJob* job = static_cast<CompressionJob*>(data);
    u32 stored_size = job->OriginalSize;
    int result = compress2(job->BufferOut, &stored_size, job->Buffer, job->OriginalSize, 9);
    if (result != 0 || stored_size >= job->OriginalSize)
        stored_size = 0;

    job->CompressResult->StoredSize = stored_size;
    job->CompressResult->Result = stored_size != 0 ? 1 : 2;
    job->Done->Increment(1);
}

DecompressionJob::DecompressionJob(CMMSemaphore* done, const CBaseVector<char>* vector, u32 load_pos, u16 offset, u16 stored_size) :
    UncompressResult(static_cast<SUncompressJobResult*>(GOtherBucket.AlignedMalloc(0x80, 0x80))),
    Buffer(0),
    StoredSize(offset),
    OriginalSize(stored_size),
    Vector(vector),
    Done(done),
    JobID(0),
    LoadPos(load_pos),
    Offset(0),
    State(offset == stored_size ? CJS_DONE : CJS_BEGIN),
    Next()
{
    UncompressResult->Result = 0;
}

DecompressionJob::~DecompressionJob()
{
    GOtherBucket.AlignedFree(UncompressResult);
    GOtherBucket.AlignedFree(Buffer);
}

void DecompressionJob::EnqueueForDecompress(bool high_priority, u32 tag)
{
    Buffer = GOtherBucket.AlignedMalloc(OriginalSize, 0x80);
    UncompressResult->Result = 0;
    State = CJS_WORKING;
    if (high_priority) {
        UncompressOnSPU(Buffer, OriginalSize, Vector->Data + LoadPos, StoredSize, UncompressResult);
    } else {
        JobID = GJobManager->EnqueueJob(1000, DecompressionJob::DecompressJob, this, tag, "DecompressionJob");
    }
}

void DecompressionJob::DecompressJob(void* data)
{
    DecompressionJob* job = static_cast<DecompressionJob*>(data);
    u32 size = job->OriginalSize;
    int result = uncompress(job->Buffer, &size, job->Vector->Data + job->LoadPos, job->StoredSize);
    if (result == 0 && job->OriginalSize == size)
        job->UncompressResult->Result = 2;
    else
        job->UncompressResult->Result = 1;
    job->Done->Increment(1);
}

u32 DecompressionJob::Finalise()
{
    State = CJS_DONE;
    return UncompressResult->Result == 2 ? 0 : 1017;
}

bool DecompressionJob::IsFinished() const
{
    if (State != CJS_WORKING)
        return State == CJS_DONE;

    if (UncompressResult->Result == 0)
        return false;

    if (JobID == 0)
        return true;

    return GJobManager->CountJobsWithJobID(JobID) == 0;
}
