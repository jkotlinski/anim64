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

#include <assert.h>
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FILE_COUNT 24
#define FILENAME_LENGTH 10
static char filename[FILE_COUNT][FILENAME_LENGTH];
static unsigned int duration[FILE_COUNT];
static unsigned char speed[FILE_COUNT];

static char selected_file;
/* 0 = file name
 * 1 = duration
 * 2 = speed */
#define MAX_COLUMN 2
static char selected_column;

#define DURATION_X (FILENAME_LENGTH + 1) 
#define SPEED_X (DURATION_X + 6)

static unsigned char update_color(unsigned char column, unsigned char row) {
    const unsigned char color = (row != selected_file || column != selected_column)
        ? (row & 1) ? COLOR_GRAY1 : COLOR_GRAY2
        : COLOR_GREEN;
    textcolor(color);
    return color;
}

static void draw_headers() {
    textcolor(COLOR_BLUE);
    gotoxy(0, 0);
    cputs("file");
    gotoxy(DURATION_X, 0);
    cputs("dur.");
    gotoxy(SPEED_X, 0);
    cputs("spd");
}

static void draw_row(unsigned char row) {
    const char y = row + 1;
    // Prints file name.
    const char color = update_color(0, row);
    revers(1);
    memset((char*)(0xd800 + y * 40), color, FILENAME_LENGTH);
    memset((char*)(0x400 + y * 40), 0xa0, FILENAME_LENGTH);
    gotoxy(0, y);
    cputs(filename[row]);
    cclear(1);
    // Prints duration.
    update_color(1, row);
    gotoxy(DURATION_X, y);
    cprintf("%5u", duration[row]);
    revers(0);
    cclear(1);
    revers(1);
    // Prints speed.
    update_color(2, row);
    gotoxy(SPEED_X, y);
    cprintf("%3i", speed[row]);
    revers(0);
}

static void draw_fields() {
    unsigned char file_it;
    for (file_it = 0; file_it < FILE_COUNT; ++file_it) {
        draw_row(file_it);
    }
}

static void show_screen() {
    draw_headers();
    draw_fields();
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

static void edit_field() {
    gotoy(selected_file + 1);
    revers(1);
    cursor(1);
    textcolor(COLOR_GREEN);
    switch (selected_column) {
        case 0:  // File.
            gotox(0);
            break;
        case 1:  // Duration.
            gotox(DURATION_X);
            cclear(5);
            gotox(DURATION_X);
            cscanf("%5u", &duration[selected_file]);
            break;
        case 2:  // Speed.
            gotox(SPEED_X);
            break;
    }
    draw_fields();
}

void edit_movie() {
    init();
    show_screen();
    while (1) {
        if (kbhit()) {
            switch (cgetc()) {
                case CH_CURS_DOWN:
                    if (selected_file < FILE_COUNT - 1) {
                        ++selected_file;
                        draw_row(selected_file - 1);
                        draw_row(selected_file);
                    }
                    break;
                case CH_CURS_UP:
                    if (selected_file) {
                        --selected_file;
                        draw_row(selected_file + 1);
                        draw_row(selected_file);
                    }
                    break;
                case CH_CURS_RIGHT:
                    if (selected_column < MAX_COLUMN) {
                        ++selected_column;
                        draw_row(selected_file);
                    }
                    break;
                case CH_CURS_LEFT:
                    if (selected_column) {
                        --selected_column;
                        draw_row(selected_file);
                    }
                    break;
                case CH_ENTER:
                    edit_field();
                    break;
                /*
                gotoxy(0, 1);
                revers(1);
                fgets(filename[0], FILENAME_LENGTH, stdin);
                ++*(char*)0xd020;
                revers(0);
                */
            }
        }
    }
}
