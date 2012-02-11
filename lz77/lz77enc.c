#include <assert.h>
#include <stdio.h>
#include <string.h>

#define ENCODED_FLAG 0x60

unsigned char org[0x2000];
unsigned char packed[0x2000];

int write_ptr = 0;
int read_ptr = 0;

/* Backpointer is 3 bytes:
 *   ENCODED_FLAG
 *   bytes back (stored -1, as it will start at least one back)
 *   length (stored -4, as 4 is the minimum useful length)
 */

static void copy_byte() {
    printf("%#x\n", org[read_ptr]);
    packed[write_ptr++] = org[read_ptr++];
}

unsigned char match_ptr;
unsigned char match_length;
unsigned char match_found;

static void find_match_length(unsigned char start) {
    const int start_offset = read_ptr - start - 1;
    unsigned int read_span = read_ptr - start_offset;
    unsigned int length = 0;
    match_found = 0;
    if (start_offset < 0) {
        return;
    }
    while (1) {
        unsigned int cyclic_read_ptr = start_offset + length;
        while (cyclic_read_ptr >= read_ptr) {
            cyclic_read_ptr -= read_ptr;
        }
        if (org[cyclic_read_ptr] == org[read_ptr + length]) {
            if (length == 255 + 4) {
                break;  // Maximum match made.
            }
            ++length;
        } else {
            break;  // Can't match longer.
        }
    }
    match_found = (length >= 4);
    match_length = length - 4;
}

static void find_pointer() {
    unsigned char window_ptr_it = 0;
    while (1) {
        unsigned char length_it = 0;
        // Check all window_ptr_it from 0..255
        find_match_length(window_ptr_it);
        if (match_found) {
            printf("%i %i\n", match_ptr - 1, match_length + 4);
        }
        if (++window_ptr_it == 0) break;
    }
}

static void pack() {
    copy_byte();
    find_pointer();
}

int main() {
    FILE* f = fopen("004.bin", "rb");
    int i;
    fread(org, 1, sizeof(org), f);
    fclose(f);

    pack();
    return 0;
}
