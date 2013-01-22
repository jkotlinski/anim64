#include <assert.h>
#include <stdio.h>
#include <string.h>

#define ENCODED_FLAG 0x60
#define SCREEN_SIZE 1501

int write_index;
int read_index;

/* Backpointer is 3 bytes: ENCODED_FLAG, distance, length.
 * ENCODED_FLAG bytes are encoded as ENCODED_FLAG, 0xff.
 */

static int match_length(const unsigned char* src, int start_index) {
    int length = 0;
    int match_index = start_index;
    while (1) {
        assert(match_index >= 0);
        assert(match_index < SCREEN_SIZE);
        assert(read_index + length >= 0);
        assert(read_index + length < SCREEN_SIZE);
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
        if (read_index + length == SCREEN_SIZE) {
            break;
        }
    }
    return length;
}

static void pack(unsigned char* dst, const unsigned char* src) {
    read_index = 0;
    write_index = 0;

    while (read_index < SCREEN_SIZE) {
        int best_match_index = -1;
        int best_length = -1;
        for (int match_index = read_index - 1;
                match_index >= 0 && match_index >= read_index - 0xfe;
                --match_index) {
            int length = match_length(src, match_index);
            if (length > best_length) {
                best_match_index = match_index;
                best_length = length;
            }
        }
        if (best_length > 3) {
            int distance = read_index - best_match_index;
            assert(distance < 0xff);
            printf("[%x %x]", distance, best_length);
            dst[write_index++] = ENCODED_FLAG;
            dst[write_index++] = distance;
            dst[write_index++] = best_length;
            read_index += best_length;
        } else {
            char byte = src[read_index++];
            printf("%x ", 0xff & byte);
            dst[write_index++] = byte;
            if (byte == ENCODED_FLAG) {
                dst[write_index++] = 0xff;
            }
        }
    }
}

void unpack(unsigned char* dst, const unsigned char* src) {
    read_index = 0;
    write_index = 0;

    while (write_index < SCREEN_SIZE) {
        if (src[read_index] == ENCODED_FLAG) {
            if (src[read_index + 1] == 0xff) {
                dst[write_index++] = ENCODED_FLAG;
                read_index += 2;
            } else {
                // Unroll...
                unsigned char distance = src[read_index + 1];
                unsigned char length = src[read_index + 2];
                const int copy_end = write_index;
                const int copy_start = write_index - distance;
                int copy_index = copy_start;

                printf("[ ");
                while (length--) {
                    assert(copy_index >= 0);
                    assert(copy_index < copy_end);
                    printf("%x ", dst[copy_index]);
                    dst[write_index++] = dst[copy_index];
                    if (++copy_index == copy_end) {
                        copy_index = copy_start;
                    }
                }
                printf("] ");
                read_index += 3;
            }
        } else {
            printf("%x ", src[read_index]);
            dst[write_index++] = src[read_index++];
        }
    }
}

int main() {
    unsigned char original[SCREEN_SIZE];
    unsigned char packed[SCREEN_SIZE];
    unsigned char unpacked[SCREEN_SIZE];

    FILE* f = fopen("wolf7,u", "rb");
    int read = fread(original, 1, sizeof(original), f);
    assert(read == SCREEN_SIZE);
    fclose(f);

    pack(packed, original);
    printf("\n%i\n", write_index);

    /* Test unpacking. */
    unpack(unpacked, packed);

    assert(0 == memcmp(unpacked, original, sizeof(unpacked)));

    return 0;
}
