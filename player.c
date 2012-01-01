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

#include "player.h"

#include <stdio.h>
#include <conio.h>

#include "irq.h"

static void load_music() {
    FILE* f = fopen("music", "r");

    // In case fread fails, insert dummy rts at init+tick.
#define RTS_OP 0x60
    *(char*)0x1000 = RTS_OP;
    *(char*)0x1003 = RTS_OP;

    fread(MUSIC_START, 1, MUSIC_STOP - MUSIC_START, f);
    fclose(f);
}

typedef void (*voidFn)(void);

#define RASTER_LINE 0xfb

void init_play() {
    *(char*)0xdc0d = 0x7f;  // Disable kernal timer interrupts.
    *(char*)1 = 0x35;  // Switch out kernal - enables $e000-$ffff RAM.

    // Scan all keyboard rows.
    *(char*)0xdc00 = 0;

    *(char*)0xd011 &= 0x7f;  // clear raster line bit 8
    *(char*)0xd012 = RASTER_LINE;  // raster line
    *(voidFn*)0xfffe = irq_handler;  // set irq handler pointer
}

void exit_play() {
    *(char*)0xd01a = 0;  // disable raster interrupts
    caught_irqs = 0;
    *(voidFn*)0xfffe = (voidFn)0x314;  // set irq handler pointer
    *(char*)1 = 0x36;  // RAM + I/O + Kernal.
    *(char*)0xdc0d = 0x81;  // Re-enable kernal timer interrupts.
    *(char*)0xd418 = 0;  // Mute sound.

    if (kbhit()) cgetc();
}

// Returns 0 if timed out, 1 if keyboard was pressed.
void play_anim(unsigned char speed, unsigned char alt_screen) {
    ticks_per_frame = speed;

    anim_screen = (alt_screen ? 8 : 0);

    caught_irqs = 1;

    *(char*)0xd01a = 1;  // enable raster interrupts
}

char wait_anim(unsigned int duration) {
    char keyboard_state = 0;

    while (duration--) {
        // Waits until raster screen is right below lower text border.
        // *(char*)0xd020 = 1;
        while (!caught_irqs) {}
        --caught_irqs;
        // *(char*)0xd020 = 0;

        // To exit animation, first all keys should be released, then
        // some key should be pressed.
        if (keyboard_state == 0) {
            if (0xff == *(char*)0xdc01) {  // All keys released?
                keyboard_state = 1;
            }
        } else if (0xff != *(char*)0xdc01) {  // Any key pressed?
            return 1;
        }
    }
    return 0;
}

