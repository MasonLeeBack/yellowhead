#include "types.h"

u64 StackTrace(u32* _pCallStack, u64 _MaxStack, u64 stack_ptr);

u64 StackTrace(u32* _pCallStack, u64 _MaxStack)
{
    register u64 Temp __asm__("r1");
    return StackTrace(_pCallStack, _MaxStack, Temp);
}

u64 StackTrace(u32* _pCallStack, u64 _MaxStack, u64 stack_ptr)
{
    u64 count = stack_ptr - stack_ptr;
    for (;;) {
        stack_ptr = *(u64*)stack_ptr;
        *_pCallStack++ = (u32)stack_ptr;
        ++count;
        if (count == _MaxStack)
            break;
        if ((u32)stack_ptr == 0)
            break;
    }
    return count;
}
