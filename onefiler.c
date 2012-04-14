/** Copyright (c) 2012, Johan Kotlinski

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

#include "onefiler.h"

#include <string.h>

#include "loops.h"
#include "movie.h"
#include "rle.h"
#include "screen.h"

#define TEST_FOO
#ifdef TEST_FOO
/* Loads a compiled movie file with the name "foo", while keeping the
 * current code intact. This #ifdef is used to make player development
 * and testing more comfortable. */
#include <cbm.h>
#include <conio.h>
static void load_foo() {
    cbm_open(8, 8, 8, "foo");
    cbm_read(8, (char*)0x7ff, 0x801 + 0x1800);  // Low-code + music.
    cbm_read(8, HEAP_START, 0x1000);  // Code - throw it away...
    cbm_read(8, HEAP_START, (char*)0xa000u - HEAP_START);  // Animation!
    cbm_close(8);
}
#endif  // TEST_FOO

extern volatile unsigned char caught_irqs;

/* $3800 - $9fff: packed screens ($6800 bytes)
 * $a000 - $a3e7: chars, screen 0
 * $a3e8 - $a3e8: bg/border, screen 0
 * $a3e9 - $a7d0: colors, screen 0
 * $a800 - $afd0: chars + border + colors, screen 1
 * $b000 - $cfff: unused
 * $e000 - $fffd: unused
 */

static void init() {
    *(char*)0xdc0d = 0x7f;  // Disable kernal timer interrupts.
    *(char*)1 = 0x35;  // Switch out kernal - enables $e000-$ffff RAM.
    *(char*)0xd011 &= 0x7f;  // clear raster line bit 8
    *(char*)0xd012 = 0xfb;  // raster line
    memset((char*)0xa000, ' ', 40 * 25);
    *(char*)0xdd00 = 0x15;  // Use graphics bank 2. ($8000-$bfff)
    *(char*)0xd018 = 0x84;  // Point video to 0xa000.
}

static void play_movie() {
    const unsigned char* anim_ptr = HEAP_START;
    const unsigned char* next_anim;
    unsigned char frame_count = anim_ptr[3];
    unsigned char anim_it = 0;
    unsigned char* write = (unsigned char*)0xa800u;

    init();

    next_anim = *(unsigned char**)anim_ptr;

    anim_ptr += 4;  // Skip size, version, frame count

    while (1) {
        anim_ptr = rle_unpack(write, anim_ptr);
        {
            const unsigned char colors = write[40 * 25];
            *(char*)0xd021 = colors;
            *(char*)0xd020 = colors >> 4;
        }

        if (anim_it) {
            if (*anim_ptr) {
                xor_v2(write, (char*)(((int)write) ^ 0x800));
            }
            ++anim_ptr;
        }
        if (++anim_it == frame_count) {
            anim_ptr = next_anim;
            next_anim = *(unsigned char**)anim_ptr;
            if (!next_anim) {
                anim_ptr = HEAP_START;
                next_anim = *(unsigned char**)HEAP_START;
            }
            anim_it = 0;
            frame_count = anim_ptr[3];
            anim_ptr += 4;  // Skip size, version, frame count
        } 

        *(char*)0xd018 ^= 0x20;  // Point video to 0xa000/0xa800.
        copy_colors_to_d800(write + 40 * 25 + 1);
        
        write ^= 0x800;
    }
}

void play_movie_if_onefiler() {
#ifdef TEST_FOO
    load_foo();
#endif  // TEST_FOO
    if (movie.speed[0]) {
        play_movie();  // load_foo may fail if this function has local variables.
    }
}

    /*
    unsigned int wait_duration = 0;
    unsigned char file_it = 0;
    unsigned char alt_screen = 0;
    init_music();
    init_play();
    for (;;) {
        // unpack_anim(file_it, alt_screen);
        while (wait_duration--) {
            while (!caught_irqs) {
                blink_vic_from_sid();
            }
            --caught_irqs;
        }
        play_anim(movie.speed[file_it], alt_screen);
        wait_duration = movie.frames[file_it] * movie.speed[file_it];

        ++file_it;
        if (file_it == FILE_COUNT || start[file_it] == 0) {
            file_it = 0;
        }
        alt_screen ^= 1;
    }
    */

