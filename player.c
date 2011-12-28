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

#define BORDER_OFFSET (40 * 25)
#define BG_OFFSET (40 * 25 + 1)

/* Defined in colcpy.s. */
void colcpy_9000();
void colcpy_9400();
void colcpy_9800();
void colcpy_9c00();

static unsigned char anim_screen;
static void anim_next_screen() {
    unsigned char* base = (char*)(0x8000 + anim_screen * 0x400);
    *(char*)0xd018 = 4 | (anim_screen << 4);  // Point video to 0x8000.
    *(char*)0xd020 = base[BORDER_OFFSET];
    *(char*)0xd021 = base[BG_OFFSET];
    switch (anim_screen) {
        case 0: colcpy_9000(); break;
        case 1: colcpy_9400(); break;
        case 2: colcpy_9800(); break;
        case 3: colcpy_9c00(); break;
    }
    ++anim_screen;
    anim_screen &= 3;
}

static void load_music() {
    FILE* f = fopen("music", "r");

    // In case fread fails, insert dummy rts at init+tick.
#define RTS_OP 0x60
    *(char*)0x1000 = RTS_OP;
    *(char*)0x1003 = RTS_OP;

    fread(MUSIC_START, 1, MUSIC_STOP - MUSIC_START, f);
    fclose(f);
}

// Defined in music.s.
void init_music();
void tick_music();

// Defined in irq.s.
typedef void (*voidFn)(void);
void irq_handler();

#define RASTER_LINE 0xfb

static void init_irq() {
    // *(char*)0xdc0d = 0x7f;  // disable interrupts
    *(char*)0xd011 &= 0x7f;  // clear raster line bit 8
    *(char*)0xd012 = RASTER_LINE;  // raster line
    *(voidFn*)0x314 = irq_handler;  // set irq handler pointer
    *(char*)0xd01a = 1;  // enable raster interrupts
    // init done!
}

extern char caught_irqs;  // Defined in irq.s.

static void play(unsigned char speed, unsigned int duration, unsigned int skipmusicframes) {
    char keyboard_state = 0;
    char delay = speed;

    init_music();
    while (skipmusicframes--) {
        tick_music();
    }

    anim_screen = 0;

    // Disable kernal timer interrupts.
    *(char*)0xdc0d = 0x7f;
    // Scan all keyboard rows.
    *(char*)0xdc00 = 0;
    *(char*)0xdd00 = 0x15;  // Use graphics bank 2. ($8000-$bfff)

    init_irq();

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
            break;
        }
        if (delay-- == 0) {
            anim_next_screen();
            delay = speed;
        }
    }

    *(char*)0xd01a = 0;  // disable raster interrupts
    *(voidFn*)0x314 = (voidFn)0xea31;  // set irq handler pointer
    // Re-enable kernal timer interrupts.
    *(char*)0xdc0d = 0x81;
    *(char*)0xd418 = 0;  // Mute sound.

    if (kbhit()) cgetc();
}

