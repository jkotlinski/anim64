#include <stdio.h>
#include <string.h>

#define ENCODED_FLAG 0x60

const unsigned char* unpack(unsigned char* dst, const unsigned char* src, int dst_size);
unsigned int pack(unsigned char* dst, const unsigned char* src, int src_size);
