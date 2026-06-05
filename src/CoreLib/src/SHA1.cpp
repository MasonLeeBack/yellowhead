#include "SHA1.h"

void sha1_init(SHA1_CONTEXT* context)
{
}

void transform(SHA1_CONTEXT* context, u8* data)
{
}

void sha1_write(SHA1_CONTEXT* context, u8* data, u32 len)
{
}

void sha1_final(SHA1_CONTEXT* context)
{
}

void CSHA1Context::Reset()
{
}

void CSHA1Context::AddData(const u8* data, u32 len)
{
}

void CSHA1Context::Result(u8* digest)
{
}
