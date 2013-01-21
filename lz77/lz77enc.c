#include <assert.h>
#include <stdio.h>
#include <string.h>

#define ENCODED_FLAG 0x60
#define SCREEN_SIZE 1501

unsigned char org[SCREEN_SIZE];
unsigned char packed[SCREEN_SIZE];

int write_index = 0;
int read_index = 0;

/* Backpointer is 3 bytes: ENCODED_FLAG, distance, length.
 * ENCODED_FLAG bytes are encoded as ENCODED_FLAG, 0xff.
 */

static void write_byte(unsigned char byte) {
    packed[write_index++] = byte;
}
static void copy_byte() {
    char byte = org[read_index++];
    printf("%x ", 0xff & byte);
    write_byte(byte);
    if (byte == ENCODED_FLAG) {
        write_byte(0xff);
    }
}

static int match_length(int start_index) {
    int length = 0;
    int match_index = start_index;
    while (1) {
        assert(match_index >= 0);
        assert(match_index < SCREEN_SIZE);
        assert(read_index + length >= 0);
        assert(read_index + length < SCREEN_SIZE);
        if (org[match_index] != org[read_index + length]) {
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

static void pack() {
    while (read_index < SCREEN_SIZE) {
        int best_match_index = -1;
        int best_length = -1;
        for (int match_index = read_index - 1;
                match_index >= 0 && match_index >= read_index - 0xfe;
                --match_index) {
            int length = match_length(match_index);
            if (length > best_length) {
                best_match_index = match_index;
                best_length = length;
            }
        }
        if (best_length > 3) {
            int distance = read_index - best_match_index;
            assert(distance < 0xff);
            printf("[%x %x]", best_match_index, best_length);
            write_byte(ENCODED_FLAG);
            write_byte(distance);
            write_byte(best_length);
            read_index += best_length;
        } else {
            copy_byte();
        }
    }
}

int main() {
    FILE* f = fopen("wolf7,u", "rb");
    int read = fread(org, 1, sizeof(org), f);
    assert(read == SCREEN_SIZE);
    fclose(f);

    pack();

    printf("\n%i", write_index);
    return 0;
}
