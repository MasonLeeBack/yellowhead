#include "GUIDHash.h"

#include <string.h>

const CGUID CGUID::ZERO = CGUID();
const CHash CHash::ZERO = CHash();
const CHash CHash::EMPTY_STRING = CHash("", 0);

bool CHash::IsSet() const
{
    return memcmp(Bytes, ZERO.Bytes, sizeof(Bytes)) != 0;
}
