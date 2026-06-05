#include "types.h"

u64 StackTrace(u32* stack_trace, u64 max_stack, u64 stack_ptr);

u64 StackTrace(u32* stack_trace, u64 max_stack)
{
    register u64 stack_ptr __asm__("r1");
    return StackTrace(stack_trace, max_stack, stack_ptr);
}

u64 StackTrace(u32* stack_trace, u64 max_stack, u64 stack_ptr)
{
    u64 count = stack_ptr - stack_ptr;
    for (;;) {
        stack_ptr = *(u64*)stack_ptr;
        *stack_trace++ = (u32)stack_ptr;
        ++count;
        if (max_stack == count)
            break;
        if ((u32)stack_ptr == 0)
            break;
    }
    return count;
}
