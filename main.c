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

#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "anim.h"
#include "anim_play.h"
#include "diff_asm.h"
#include "edit.h"
// #include "effects.h"
#include "irq.h"
#include "movie.h"
#include "music.h"
#include "player.h"
#include "screen.h"

#define RLE_BUFFER_V2 (unsigned char*)0xc800u
#define RLE_BUFFER_SIZE_V2 0x800u

static void init() {
    clrscr();
    bordercolor(0);
    bgcolor(0);

    // *(char*)0xdd00 = 0x15;  // Use graphics bank 2. ($8000-$bfff)
    // *(char*)0xd018 = 4;  // Point video to 0x8000.
    *(char*)0xd018 = 0x14;  // Point video to 0x400.
}

#pragma codeseg("EDITCODE")
#pragma rodataseg("EDITCODE")

void edit() {
    unsigned char screen;
    textcolor(COLOR_YELLOW);

    for (screen = 0; screen < 16; ++screen) {
        clear_screen(screen);
    }

    anim_edit_loop();
}

#pragma codeseg("CODE")
#pragma rodataseg("CODE")

void main() {
#if 0
    while(1) { if (kbhit()) { printf("%x", cgetc()); } }
#endif
    init();

    play_movie_if_onefiler();

    edit();
}
