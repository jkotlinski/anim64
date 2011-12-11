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
#include <string.h>

#define FILE_COUNT 24
#define FILENAME_LENGTH 16
static char filename[FILE_COUNT][FILENAME_LENGTH];
static unsigned int duration[FILE_COUNT];

static char selected_file;

#define DURATION_X 17

static void show_screen() {
    unsigned char file_it;
    textcolor(COLOR_BLUE);
    gotoxy(0, 0);
    cputs("file");
    gotoxy(DURATION_X, 0);
    cputs("dur.");
    for (file_it = 0; file_it < FILE_COUNT; ++file_it) {
        const char y = file_it + 1;
        const char color = (file_it == selected_file) ? 1 : (file_it & 1 ? COLOR_GRAY1 : COLOR_GRAY2);
        memset((char*)(0xd800 + y * 40), color, 16);
        memset((char*)(0x400 + y * 40), 0xa0, 16);
        memset((char*)(0xd800 + DURATION_X + y * 40), color, 5);
        memset((char*)(0x400 + DURATION_X + y * 40), 0xa0, 5);
        // Prints file name.
        textcolor(color);
        gotoxy(0, y);
        cputs(filename[file_it]);
        // Prints duration.
        gotoxy(DURATION_X, y);
        revers(1);
        cprintf("%5i", duration[file_it]);
    }
}

static void init() {
    char file_it;
    static char inited;
    if (inited) {
        return;
    }
    for (file_it = 0; file_it < FILE_COUNT; ++file_it) {
        duration[file_it] = 100;
    }
    inited = 1;
}

void edit_movie() {
    init();
    show_screen();
    while (1) {
        if (kbhit()) {
        }
    }
}
