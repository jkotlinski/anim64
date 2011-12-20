/** Copyright (c) 2011, Johan Kotlinski

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

#include <assert.h>

unsigned int pack(char* src, char* dst, unsigned int src_size) {
    const char* start = dst;
    unsigned int run = 0;
    char prev = *src;

    while (src_size--) {
        const char curr = *(src++);
        if (curr == prev) {
            ++run;
        } else {
            // Run ended.
write_remaining:
            assert(run);
            if (run == 1) {
                *(dst++) = prev;
            } else {
                run -= 2;  // Always write at least two bytes.
                *(dst++) = prev;
                *(dst++) = prev;
                if (run < 0x100) {
                    *(dst++) = run;
                } else {
                    *(dst++) = 0xff;
                    run -= 0xff;
                    goto write_remaining;
                }
            }
            run = 1;
            prev = curr;
        }
    }
    return dst - start;
}

unsigned int unpack(char* src, char* dst, unsigned int src_size) {
    (void)src;
    (void)dst;
    (void)src_size;
    return 0;
}
