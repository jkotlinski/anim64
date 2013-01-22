#include "lz77.h"

#include "lz77_dec.c"
#include "lz77_enc.c"

#include <assert.h>
#include <stdio.h>

#define SCREEN_SIZE 1501

int main() {
    unsigned char original[SCREEN_SIZE];
    unsigned char packed[SCREEN_SIZE];
    unsigned char unpacked[SCREEN_SIZE];

    FILE* f = fopen("wolf7,u", "rb");
    int read = fread(original, 1, sizeof(original), f);
    assert(read == SCREEN_SIZE);
    fclose(f);

    printf("%i", lz77_pack(packed, original));

    /* Test unpacking. */
    lz77_unpack(unpacked, packed);

    assert(0 == memcmp(unpacked, original, sizeof(unpacked)));

    return 0;
}
