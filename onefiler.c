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

#include "irq.h"
#include "loops.h"
#include "movie.h"
#include "music.h"
#include "player.h"
#include "rle.h"
#include "screen.h"

// #define TEST_FOO
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
    cbm_read(8, HEAP_START, (char*)0xd000u - HEAP_START);  // Animation!
    cbm_close(8);
}
#endif  // TEST_FOO

extern volatile unsigned char caught_irqs;

/*  $400 -  $7ff: temp color buffer
 *  $800 - $3bff: code & music
 * $3c00 - $cfff: packed screens ($9800 bytes)
 * $e000 - $efff: copy of character ROM
 * $f000 - $f3e7: chars, screen 0
 * $f3e8 - $f3e8: bg/border, screen 0
 * $f3e9 - $f7d0: colors, screen 0
 * $f800 - $ffd0: chars + border + colors, screen 1
 */

static void init() {
    *(char*)0xdc0d = 0x7f;  // Disable kernal timer interrupts.
    *(char*)1 = 0x31;  // RAM + character ROM + RAM.
    memcpy((char*)0xe000, (char*)0xd000, 0x1000);  // Copy character ROM to RAM.
    *(char*)1 = 0x35;  // RAM + I/O + RAM.
    *(char*)0xd011 &= 0x7f;  // clear raster line bit 8
    *(char*)0xd012 = 0xfb;  // raster line
    memset((char*)0xf000, ' ', 40 * 25);
    init_music();
    *(char*)0xdd00 = 0x14;  // Use graphics bank 3. ($c000-$ffff)
    *(char*)0xd018 = 0xc8;  // Point video to $f000, char to $e000.
    *(voidFn*)0xfffe = irq_handler_v2;  // set irq handler pointer
}

static void play_movie() {
    const unsigned char* anim_ptr = HEAP_START;
    const unsigned char* anim_start_ptr = HEAP_START + 4;
    const unsigned char* next_anim;
    unsigned char anim_frame_count = anim_ptr[3];
    unsigned char anim_it = 0;
    unsigned char anim_frame_it = 0;
    unsigned char* write = (unsigned char*)0xf800u;
    unsigned char first_tick = 1;
    unsigned char frames_left = movie.frames[0];

    init();

    next_anim = *(unsigned char**)anim_ptr;

    anim_ptr += 4;  // Skip size, version, frame count

    while (1) {
        anim_ptr = rle_unpack(write, anim_ptr);

        // Handles XOR.
        if (anim_frame_it) {
            if (*anim_ptr) {
                xor_v2(write, (char*)(((int)write) ^ 0x800));
            }
            ++anim_ptr;
        }
        
        unpack_colors((char*)0x400u, write + 40 * 25 + 1);

        if (first_tick) {
            *(char*)0xd01a = 1;  // Enables raster interrupts.
            first_tick = 0;
        } else {
            // Waits for enough ticks...
            unsigned char count = movie.speed[anim_it];
            while (count) {
                if (caught_irqs) {
                    --caught_irqs;
                    --count;
                } else {
                    blink_vic_from_sid();
                }
            }
        }

        // Shows new frame.
        *(char*)0xd018 ^= 0x20;  // Point video to 0xf000/0xf800.

        // Copies colors.
        memcpy((char*)0xd800, (char*)0x400u, 40 * 25);
        {
            unsigned char colors = write[40 * 25];
            *(char*)0xd021 = colors;
            *(char*)0xd020 = colors >> 4;
        }

        write ^= 0x800;

        // Update pointers if we reached animation end.
        if (!--frames_left) {
            ++anim_it;
            anim_frame_it = 0;
            anim_ptr = next_anim;
            next_anim = *(unsigned char**)anim_ptr;
            if (!next_anim) {
                // EOF found, restart movie.
                anim_it = 0;
                anim_ptr = HEAP_START;
                next_anim = *(unsigned char**)HEAP_START;
            }
            anim_frame_count = anim_ptr[3];
            anim_ptr += 4;  // Skips size, version, frame count.
            anim_start_ptr = anim_ptr;
            frames_left = movie.frames[anim_it];
        } else if (++anim_frame_it == anim_frame_count) {
            anim_frame_it = 0;
            anim_ptr = anim_start_ptr;
        } 
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
