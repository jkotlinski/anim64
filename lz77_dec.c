#include "lz77.h"

const unsigned char* unpack(unsigned char* dst, const unsigned char* src, unsigned int dst_size) {
    unsigned int write_index = 0;

    while (write_index < dst_size) {
        if (*src == ENCODED_FLAG) {
            if (src[1] == 0xff) {
                dst[write_index] = ENCODED_FLAG;
                ++write_index;
                src += 2;
            } else {
                // Unroll...
                unsigned char distance = src[1];
                unsigned char length = src[2];
                const unsigned int copy_end = write_index;
                const unsigned int copy_start = write_index - distance;
                unsigned int copy_index = copy_start;

                do {
                    dst[write_index] = dst[copy_index];
                    ++write_index;
                    ++copy_index;
                    if (copy_index == copy_end) {
                        copy_index = copy_start;
                    }
                } while (--length);

                src += 3;
            }
        } else {
            dst[write_index] = *src;
            ++src;
            ++write_index;
        }
    }
    return src;
}
