#include "GUIDHash.h"

const CGUID CGUID::ZERO = CGUID();
const CHash CHash::ZERO = CHash();
const CHash CHash::EMPTY_STRING = CHash();

bool CHash::IsSet() const
{
    return false;
}
