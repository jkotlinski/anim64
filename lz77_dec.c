/** Copyright (c) 2013, Johan Kotlinski

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE. */

#include "lz77.h"

const unsigned char* lz77_unpack(unsigned char* dst, const unsigned char* src) {
    unsigned int write_index = 0;

    do {
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
    } while(write_index < PACK_SIZE);

    return src;
}
