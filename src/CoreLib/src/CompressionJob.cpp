#include "CompressionJob.h"

CompressionJob::CompressionJob(CMMSemaphore* done, u32 buffer_size, u32 level) :
    CompressResult(0),
    BufferSize(buffer_size),
    Buffer(0),
    BufferOut(0),
    OriginalSize(0),
    StoredSize(0),
    Done(done),
    JobID(0),
    Level(level),
    State(CJS_Empty),
    Prev(),
    Next()
{
}

CompressionJob::~CompressionJob()
{
}

bool CompressionJob::IsFinished() const
{
    return State == CJS_Finished;
}

void CompressionJob::Finalise(ByteArray& out)
{
    State = CJS_Finished;
}

void CompressionJob::EnqueueForCompress(bool high_priority, u32 tag)
{
    State = CJS_Queued;
    JobID = tag;
}

void CompressionJob::CompressJob(void* data)
{
    CompressionJob* job = static_cast<CompressionJob*>(data);
    if (job)
        job->State = CJS_Finished;
}

DecompressionJob::DecompressionJob(CMMSemaphore* done, const CBaseVector<char>* vector, u32 load_pos, u16 offset, u16 stored_size) :
    UncompressResult(0),
    Buffer(0),
    StoredSize(stored_size),
    OriginalSize(0),
    Vector(vector),
    Done(done),
    JobID(0),
    LoadPos(load_pos),
    Offset(offset),
    State(CJS_Empty),
    Next()
{
}

DecompressionJob::~DecompressionJob()
{
}

void DecompressionJob::EnqueueForDecompress(bool high_priority, u32 tag)
{
    State = CJS_Queued;
    JobID = tag;
}

void DecompressionJob::DecompressJob(void* data)
{
    DecompressionJob* job = static_cast<DecompressionJob*>(data);
    if (job)
        job->State = CJS_Finished;
}

void DecompressionJob::Finalise()
{
    State = CJS_Finished;
}

bool DecompressionJob::IsFinished() const
{
    return State == CJS_Finished;
}
