#include "types.h"

#include <string.h>

u64 StackTrace(u32* stack_trace, u64 max_stack);

u32 StackTrace_FillStackTrace(u32* trace, u32 max_entries)
{
    u32 stacktrace_length = (u32)StackTrace(trace, max_entries);
    if (stacktrace_length) {
        --stacktrace_length;
        if (stacktrace_length) {
            for (u64 i = 0; i != stacktrace_length; ++i) {
                u64* pp = (u64*)trace[i];
                if (pp)
                    trace[i] = (u32)pp[2];
            }
        }
    }
    return stacktrace_length;
}

u32 StackTrace_FillStackTrace(u32* trace, u32 max_entries, u32 return_address)
{
    u32 count = StackTrace_FillStackTrace(trace, max_entries);
    if (count) {
        for (u32 i = 0; i != count; ++i) {
            if (trace[i] == return_address) {
                if (i) {
                    count -= i;
                    memmove(trace, trace + i, count * sizeof(u32));
                }
                break;
            }
        }
    }
    return count;
}
