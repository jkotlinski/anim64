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

#include "rle.h"
#include "player.h"

#define RLE_BUFFER (unsigned char*)0xa000u
#define VIDEO_BASE (unsigned char*)0x8000u

#define FILE_COUNT 24
#define FILENAME_LENGTH 8
static struct Movie {
    char filename[FILE_COUNT][FILENAME_LENGTH];
    unsigned int duration[FILE_COUNT];
    unsigned char speed[FILE_COUNT];
} movie;

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
    cputs(movie.filename[row]);
    gotox(DURATION_X - 1);
    revers(0);
    cclear(1);
    revers(1);
    // Prints duration.
    update_color(1, row);
    gotox(DURATION_X);
    cprintf("%5u", movie.duration[row]);
    revers(0);
    cclear(1);
    revers(1);
    // Prints speed.
    update_color(2, row);
    gotox(SPEED_X);
    cprintf("%3i", movie.speed[row]);
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

static const char* MOVIE_FILE = "movie";

static void load_movie() {
    FILE* f = fopen(MOVIE_FILE, "r");
    if (!f) return;
    fread(&movie, sizeof(movie), 1, f);
    fclose(f);
}

static void save_movie() {
    FILE* f;
    gotoxy(20, 0);
    printf("save...");
    f = fopen(MOVIE_FILE, "w");
    fwrite(&movie, sizeof(movie), 1, f);
    fclose(f);
    printf("ok");
}

static void init() {
    char file_it;
    static char inited;
    if (inited) {
        return;
    }
    for (file_it = 0; file_it < FILE_COUNT; ++file_it) {
        movie.duration[file_it] = 100;
    }
    load_movie();
    inited = 1;
}

static void edit_field() {
    gotoy(selected_file + 1);
    revers(1);
    cursor(1);
    textcolor(COLOR_RED);
    switch (selected_column) {
        case 0:  // File.
            gotox(0);
            cclear(FILENAME_LENGTH);
            gotox(0);
            cscanf("%8s", &movie.filename[selected_file]);
            break;
        case 1:  // Duration.
            gotox(DURATION_X);
            cclear(5);
            gotox(DURATION_X);
            cscanf("%5u", &movie.duration[selected_file]);
            break;
        case 2:  // Speed.
            gotox(SPEED_X);
            cclear(3);
            gotox(SPEED_X);
            {
                unsigned int x;
                cscanf("%3u", &x);
                movie.speed[selected_file] = (x & 0xff00u) ? 0xff : x;
            }
            break;
    }
    draw_fields();
}

static void run_anim() {
    FILE* f = fopen(movie.filename[selected_file], "r");
    if (!f) return;
    fread(RLE_BUFFER, 1, 0x3000, f);
    fclose(f);
    rle_unpack(VIDEO_BASE, RLE_BUFFER);
    play(movie.speed[selected_file], movie.duration[selected_file]);
    *(char*)0xdd00 = 0x17;  // Use graphics bank 0. ($0000-$3fff)
    *(char*)0xd018 = 0x14;  // Point video to 0x400.
    clrscr();
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
                case CH_F1: load_movie(); break;
                case CH_F2: save_movie(); break;
                case CH_STOP: run_anim(); break;
                case CH_F7:  // Go to animation editor.
                    return;
            }
        }
    }
}
