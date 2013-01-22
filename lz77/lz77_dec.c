#include "lz77.h"

#include <assert.h>

const unsigned char* unpack(unsigned char* dst, const unsigned char* src, int dst_size) {
    int write_index = 0;

    while (write_index < dst_size) {
        if (*src == ENCODED_FLAG) {
            if (src[1] == 0xff) {
                dst[write_index++] = ENCODED_FLAG;
                src += 2;
            } else {
                // Unroll...
                unsigned char distance = *++src;
                unsigned char length = *++src;
                const int copy_end = write_index;
                const int copy_start = write_index - distance;
                int copy_index = copy_start;

                // printf("[ ");
                while (length--) {
                    assert(copy_index >= 0);
                    assert(copy_index < copy_end);
                    // printf("%x ", dst[copy_index]);
                    dst[write_index++] = dst[copy_index];
                    if (++copy_index == copy_end) {
                        copy_index = copy_start;
                    }
                }
                // printf("] ");

                ++src;
            }
        } else {
            // printf("%x ", *src);
            dst[write_index++] = *src++;
        }
    }
    return src;
}
