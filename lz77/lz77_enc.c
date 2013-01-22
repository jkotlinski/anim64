#include "lz77.h"

// #include <assert.h>

#pragma codeseg("EDITCODE")
#pragma rodataseg("EDITCODE")

int read_index;

/* Backpointer is 3 bytes: ENCODED_FLAG, distance, length.
 * ENCODED_FLAG bytes are encoded as ENCODED_FLAG, 0xff.
 */

static unsigned char match_length(const unsigned char* src, int start_index) {
    unsigned char length = 0;
    unsigned int match_index = start_index;
    while (1) {
        // assert(match_index >= 0);
        // assert(match_index < src_size);
        // assert(read_index + length >= 0);
        // assert(read_index + length < src_size);
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

unsigned int lz77_pack(unsigned char* dst, const unsigned char* src) {
    unsigned int written = 0;

    read_index = 0;

    while (read_index < PACK_SIZE) {
        int best_match_index = -1;
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
            // assert(distance < 0xff);
            // printf("[%x %x]", distance, best_length);
            *dst++ = ENCODED_FLAG;
            *dst++ = read_index - best_match_index;
            *dst++ = best_length;
            written += 3;
            read_index += best_length;
        } else {
            char byte = src[read_index++];
            // printf("%x ", 0xff & byte);
            *dst++ = byte;
            ++written;
            if (byte == ENCODED_FLAG) {
                *dst++ = 0xff;
                ++written;
            }
        }
    }
    return written;
}


