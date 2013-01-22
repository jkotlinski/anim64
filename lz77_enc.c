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

#pragma codeseg("EDITCODE")
#pragma rodataseg("EDITCODE")

unsigned int read_index;

/* Backpointer is 3 bytes: ENCODED_FLAG, distance, length.
 * ENCODED_FLAG bytes are encoded as ENCODED_FLAG, 0xff.
 */

static unsigned char match_length(const unsigned char* src, unsigned int start_index) {
    unsigned char length = 0;
    unsigned int match_index = start_index;
    while (1) {
        if (src[match_index] != src[read_index + length]) {
            break;
        }
        ++length;
        ++match_index;
        if (match_index == read_index) {
            match_index = start_index;  // Reached end of window, jump back.
        }
        if (length == 0xff) {
            break;  // Reached max length!
        }
        if (read_index + length == PACK_SIZE) {
            break;
        }
    }
    return length;
}

unsigned int pack(unsigned char* dst, const unsigned char* src) {
    unsigned int written = 0;

    read_index = 0;

    while (read_index < PACK_SIZE) {
        unsigned int best_match_index;
        unsigned char best_length = 0;
        int match_index = read_index - 1;
        while (match_index >= 0 && match_index >= read_index - 0xfe) {
            unsigned char length = match_length(src, match_index);
            if (length > best_length) {
                best_match_index = match_index;
                best_length = length;
            }
            --match_index;
        }
        if (best_length > 3) {
            *dst++ = ENCODED_FLAG;
            *dst++ = read_index - best_match_index;  // Distance.
            *dst++ = best_length;
            written += 3;
            read_index += best_length;
        } else {
            char byte = src[read_index];
            ++read_index;
            *dst = byte;
            ++dst;
            ++written;
            if (byte == ENCODED_FLAG) {
                *dst = 0xff;
                ++dst;
                ++written;
            }
        }
    }
    return written;
}


