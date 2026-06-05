#pragma once

#include "types.h"

namespace Vectormath {
namespace Aos {

inline bool operator==(Vector4 a, Vector4 b)
{
    vec_uint4 eq = (vec_uint4)vec_cmpeq(a.get128(), b.get128());
    eq = vec_and(vec_splat(eq, 0), vec_splat(eq, 1));
    eq = vec_and(eq, vec_splat(eq, 2));
    eq = vec_and(eq, vec_splat(eq, 3));
    return vec_all_eq(eq, (vec_uint4)(0xffffffff));
}

inline bool operator!=(Vector4 a, Vector4 b)
{
    return !(a == b);
}

}
}
