#ifndef H_IDLTYPES
#define H_IDLTYPES

#include <sys/types.h>
#include <stdio.h>

// int16 and int32 types
#define int16 int16_t
#define int32 int32_t
#define int64 int64_t

// Bytearray structure: dat and length of data
struct bytearray
{
	int32 len;
	char* data;
};
#endif


