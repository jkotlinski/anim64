#include "lz77.h"

#include "lz77_dec.c"
#include "lz77_enc.c"

#define SCREEN_SIZE 1501

int main() {
    unsigned char original[SCREEN_SIZE];
    unsigned char packed[SCREEN_SIZE];
    unsigned char unpacked[SCREEN_SIZE];

    FILE* f = fopen("wolf7,u", "rb");
    int read = fread(original, 1, sizeof(original), f);
    assert(read == SCREEN_SIZE);
    fclose(f);

    printf("%i", pack(packed, original, SCREEN_SIZE));

    /* Test unpacking. */
    unpack(unpacked, packed, SCREEN_SIZE);

    assert(0 == memcmp(unpacked, original, sizeof(unpacked)));

    return 0;
}
