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
    unsigned char* start[FILE_COUNT];
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
    const unsigned int offs = y * 40;
    // Prints file name.
    const char color = update_color(0, row);
    revers(1);
    memset((char*)(0xd800 + offs), color, FILENAME_LENGTH);
    memset((char*)(0x400 + offs), 0xa0, FILENAME_LENGTH);
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
    clrscr();
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
    FILE* f = fopen(MOVIE_FILE, "w");
    if (!f || !fwrite(&movie, sizeof(movie), 1, f)) {
        puts("err");
    } else {
        puts("ok");
    }
    fclose(f);
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

static unsigned int skip_music_frames(unsigned char file) {
    unsigned int frames = 0;
    unsigned char file_it;
    for (file_it = 0; file_it < file; ++file_it) {
        frames += movie.duration[file];
    }
    return frames;
}

static char packed_anims_valid;

void invalidate_packed_anims() {
    packed_anims_valid = 0;
}

// Pack the different anims into RLE buffer.
void pack_anims() {
    unsigned char* rle_ptr = RLE_BUFFER;
    unsigned char anim_it;
    if (packed_anims_valid) {
        return;
    }
    for (anim_it = 0; anim_it < FILE_COUNT; ++anim_it) {
        FILE* f;
        movie.start[anim_it] = NULL;
        if (!movie.filename[anim_it][0]) {
            continue;
        }
        f = fopen(movie.filename[anim_it], "r");
        if (!f) {
            continue;
        }
        movie.start[anim_it] = rle_ptr;
        rle_ptr += fread(rle_ptr, 1, 0x3000, f);
        fclose(f);
    }
    packed_anims_valid = 1;
}

static void run_anims(unsigned char file_it) {
    init_play(skip_music_frames(file_it));
    for (;;) {
        const unsigned char* rle_data = movie.start[file_it];
        if (rle_data == NULL) {
            file_it = 0;
            continue;
        }
        rle_unpack(VIDEO_BASE, rle_data);
        if (play_anim(movie.speed[file_it], movie.duration[file_it])) {
            break;
        }
        ++file_it;
        file_it %= FILE_COUNT;
    }
    exit_play();
    *(char*)0xdd00 = 0x17;  // Use graphics bank 0. ($0000-$3fff)
    *(char*)0xd018 = 0x14;  // Point video to 0x400.
    *(char*)0xd020 = 0;
    *(char*)0xd021 = 0;
    show_screen();
}

static char handle_key(unsigned char key) {
    switch (key) {
        default:
            if (key >= '0' && key <= '9' && selected_column > 0) {
                edit_field();
            }
            break;
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
        case CH_F2:
            gotoxy(20, 0);
            printf("save...");
            pack_anims();
            save_movie();
            break;
        case CH_STOP: pack_anims(); run_anims(selected_file); break;
        case CH_F7:  // Go to animation editor.
                      return 1;
    }
    return 0;
}

void edit_movie() {
    init();
    show_screen();

    for (;;) {
        if (kbhit() && handle_key(cgetc())) {
            break;
        }
    }
}
