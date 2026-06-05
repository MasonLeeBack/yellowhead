#pragma once

#include "types.h"

enum ReflectReturn {
    REFLECT_OK = 0,
    REFLECT_GENERIC_ERROR = 1000,
    REFLECT_EXCESSIVE_DATA = 1001,
    REFLECT_INSUFFICIENT_DATA = 1002,
    REFLECT_EXCESSIVE_ALLOCATIONS = 1003,
    REFLECT_FORMAT_TOO_NEW = 1004,
    REFLECT_FORMAT_TOO_OLD = 1005,
    REFLECT_COULDNT_OPEN_FILE = 1006,
    REFLECT_FILEIO_FAILURE = 1007,
    REFLECT_NETWORK_FAILURE = 1008,
    REFLECT_NOT_IMPLEMENTED = 1009,
    REFLECT_COULDNT_GET_GUID = 1010,
    REFLECT_UNINITIALISED = 1011,
    REFLECT_NAN = 1012,
    REFLECT_INVALID = 1013,
    REFLECT_RESOURCE_IN_WRONG_STATE = 1014,
    REFLECT_OUT_OF_GFX_MEM = 1015,
    REFLECT_OUT_OF_SYNC = 1016,
    REFLECT_DECOMPRESSION_FAIL = 1017,
    REFLECT_COMPRESSION_FAIL = 1018,
    REFLECT_APPLICATION_QUITTING = 1019,
    REFLECT_OUT_OF_MEM = 1020,
};

class SRevision {
public:
    SRevision(u32 revision = 0, u32 branch_description = 0) :
        Revision(revision),
        BranchDescription(branch_description)
    {
    }

    u32 GetRevision() const { return Revision; }
    u32 GetBranchDescription() const { return BranchDescription; }
    u32 GetBranchID() const { return BranchDescription >> 16; }
    u32 GetBranchRevision() const { return BranchDescription & 0xffff; }

    bool IsAfterRevision(const SRevision& rhs) const { return Revision > rhs.Revision; }
    bool IsBeforeRevision(const SRevision& rhs) const { return Revision < rhs.Revision; }
    bool IsBetweenRevisions(const SRevision& min_revision, const SRevision& max_revision) const
    {
        return !IsBeforeRevision(min_revision) && !max_revision.IsBeforeRevision(*this);
    }

    ReflectReturn CheckRevision() const { return Revision != 0 ? REFLECT_OK : REFLECT_FORMAT_TOO_OLD; }
    ReflectReturn CheckBranchDescription() const;

private:
    u32 Revision;
    u32 BranchDescription;
};

extern SRevision GLeerdammerFormatRevision;
extern SRevision GFormatRevision;

typedef char check_srevision_size[sizeof(SRevision) == 0x8 ? 1 : -1];
