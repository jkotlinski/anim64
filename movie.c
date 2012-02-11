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

#include "diff.h"
#include "disk.h"
#include "music.h"
#include "rle.h"
#include "player.h"

#define VIDEO_BASE (unsigned char*)0x8000u

/* RAM end - $7fff: rle buffer
 * $8000 - $8fff: screen 0-3, + border/screen color
 * $9000 - $9fff: color 0-3
 * $a000 - $afff: screen 4-7, + border/screen color
 * $b000 - $bfff: color 4-7
 * $c000 - $cfff: rle buffer
 * $e000 - $ffff: rle buffer
 */

#define FILE_COUNT 20

#pragma bssseg(push, "EDITCODE")
char filename[FILE_COUNT][FILENAME_LENGTH];
char music_path[FILENAME_LENGTH];
#pragma bssseg(pop)

#pragma bssseg (push,"DATA")
/* These variables are used in onefiler mode. They are put in DATA segment
 * instead of BSS to keep them from being zero-initialized.
 */
static struct Movie {
    unsigned char frames[FILE_COUNT];
    unsigned char speed[FILE_COUNT];
} movie;
#pragma bssseg (pop)

static char selected_file;


/* 0 = file name
 * 1 = duration
 * 2 = speed */
#define MAX_COLUMN 2
static char selected_column;

#define DURATION_X (FILENAME_LENGTH + 1) 
#define SPEED_X (DURATION_X + 4)

static const char* MOVIE_FILE = ".movie";

#pragma codeseg("EDITCODE")

static void load_music(FILE* f) {
#define MUSIC_START ((char*)0x1000)
#define MUSIC_STOP ((char*)0x2800)
    if ((char)fgetc(f) == (char)MUSIC_START &&
            (char)fgetc(f) == ((char)((int)MUSIC_START >> 8))) {
        fread(MUSIC_START, 1, MUSIC_STOP - MUSIC_START, f);
    }
    fclose(f);
}

static void load_movie() {
    FILE* f = fopen(MOVIE_FILE, "r");
    if (!f) return;
    fread(&filename, sizeof(filename), 1, f);
    fread(&movie, sizeof(movie), 1, f);
    fread(&music_path, sizeof(music_path), 1, f);
    fclose(f);
    if (*music_path) {
        f = fopen(music_path, "r");
        load_music(f);
    }
}

static void save_movie() {
    FILE* f = fopen(MOVIE_FILE, "w");
    gotoxy(20, 0);
    cputs("save...");
    if (!f || !fwrite(&filename, sizeof(filename), 1, f)) {
        puts("err");
    } else {
        fwrite(&movie, sizeof(movie), 1, f);
        fwrite(music_path, sizeof(music_path), 1, f);
        puts("ok");
    }
    fclose(f);
}

static unsigned char loaded_anim[2] = { -1, -1 };

static unsigned int skip_music_frames() {
    unsigned int frames = 0;
    unsigned char file_it;
    for (file_it = 0; file_it < selected_file; ++file_it) {
        frames += movie.frames[file_it] * movie.speed[file_it];
    }
    return frames;
}

/* The following two are defined by the linker. */
extern unsigned char _EDITRAM_LAST__;
extern unsigned char _RAM_LAST__;
#define HEAP_START (char*)0x4000u

#pragma codeseg("CODE")

unsigned char* start[FILE_COUNT];

// Returns 1 if load succeeded, otherwise 0.
static void unpack_anim(char file_it, unsigned char alt_screen) {
    unsigned char* screen_base = (unsigned char*)(alt_screen ? 0xa000u : 0x8000u);
    rle_unpack(screen_base, start[file_it]);
    undiff(screen_base);
}

void show_screen();

extern volatile unsigned char caught_irqs;

void move_files_in_place() {
    unsigned char file_it = 0;
    unsigned char* head = HEAP_START;
    while (*head) {
        unsigned char* addr = (unsigned char*)((*head++ << 8) | (*head++ & 0xffu));
        unsigned int size = (*head++ << 8) | (*head++ & 0xffu);
        start[file_it++] = addr;
        if (addr != head)
            memcpy(addr, head, size);
        head += size;
    }
}

static char is_onefiler() {
    return movie.speed[0];
}

void play_movie_if_onefiler() {
    unsigned int wait_duration = 0;
    unsigned char file_it = 0;
    unsigned char alt_screen = 0;
    if (!is_onefiler()) {
        return;
    }
    move_files_in_place();
    init_music();
    init_play();
    for (;;) {
        unpack_anim(file_it, alt_screen);
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
}

#pragma codeseg("EDITCODE")

static void prompt_music() {
    FILE* f = prompt_open("music", "r");
    if (f) {
        strcpy(music_path, prompt_path);
        load_music(f);
        show_screen();
    }
}

static void read_filename() {
    char* ptr = filename[selected_file];
    char chars = 7;
    while (chars-- > 0) {
        unsigned char c = cgetc();
        if (c == CH_ENTER) {
            break;
        }
        cputc(c);
        *ptr = c;
        ++ptr;
    }
    *ptr = 0;
}

static unsigned int read_digits() {
    unsigned int number = 0;
    unsigned char digits = 3;
    while (digits > 0) {
        char c = cgetc();
        if (c >= '0' && c <= '9') {
            cputc(c);
            number *= 10;
            number += c - '0';
        } else if (c == CH_ENTER) {
            break;
        }
    }
    return number;
}
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
    cputs("dur");
    gotoxy(SPEED_X, 0);
    cputs("spd");
}

static void print_digits(unsigned int number) {
    unsigned char buf[5];
    unsigned char it;
    unsigned char filler = ' ';
    unsigned char digits = 3;
    for (it = 0; it < 3; ++it) {
        buf[it] = number % 10;
        number /= 10;
    }
    while (digits--) {
        char digit = buf[digits];
        if (digit || !digits) {
            cputc('0' + digit);
            filler = '0';
        } else {
            cputc(filler);
        }
    }
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
    cputs(filename[row]);
    gotox(DURATION_X - 1);
    revers(0);
    cclear(1);
    revers(1);
    // Prints duration.
    update_color(1, row);
    gotox(DURATION_X);
    print_digits(movie.frames[row]);
    revers(0);
    cclear(1);
    revers(1);
    // Prints speed.
    update_color(2, row);
    gotox(SPEED_X);
    print_digits(movie.speed[row]);
    revers(0);
}

static void draw_fields() {
    unsigned char file_it;
    for (file_it = 0; file_it < FILE_COUNT; ++file_it) {
        draw_row(file_it);
    }
}

static void show_screen() {
    *(char*)0xdd00 = 0x17;  // Use graphics bank 0. ($0000-$3fff)
    *(char*)0xd018 = 0x14;  // Point video to 0x400.
    *(char*)0xd021 = COLOR_BLACK;
    memset((char*)0xd800, COLOR_YELLOW, 0x400);
    clrscr();
    draw_headers();
    draw_fields();
}

static void init() {
    char file_it;
    static char inited;
    if (inited || is_onefiler()) {
        return;
    }
    /* Since movie is not in BSS, zero-init filename and speed explicitly. */
    for (file_it = 0; file_it < FILE_COUNT; ++file_it) {
        movie.frames[file_it] = 4;
        movie.speed[file_it] = 32;
    }
    load_movie();
    inited = 1;
}

void invalidate_loaded_anim() {
    loaded_anim[0] = -1;
    loaded_anim[1] = -1;
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
            read_filename();
            invalidate_loaded_anim();
            break;
        case 1:  // Duration.
            gotox(DURATION_X);
            cclear(3);
            gotox(DURATION_X);
            movie.frames[selected_file] = read_digits();
            break;
        case 2:  // Speed.
            gotox(SPEED_X);
            cclear(3);
            gotox(SPEED_X);
            {
                unsigned int x = read_digits();
                if (!x) x = 1;
                if (x & 0xff00u) x = 0xff;
                movie.speed[selected_file] = x;
            }
            break;
    }
    draw_fields();
}

static void load_selected_anim(unsigned char alt_screen) {
    FILE* f;
    char* screen = (char*)(alt_screen ? 0xa000u : 0x8000u);
    if (loaded_anim[alt_screen] == selected_file) return; 
    f = fopen(filename[selected_file], "r");
    if (!f) {
        cputs("err");
        return; 
    }
    fread(&_EDITRAM_LAST__, 1, 0x8000u - (unsigned int)&_EDITRAM_LAST__, f);
    fclose(f);
    loaded_anim[alt_screen] = selected_file;
    rle_unpack(screen, &_EDITRAM_LAST__);
    undiff(screen);
}

static unsigned int get_file_length(unsigned char file) {
    FILE* f;
    unsigned int length;
    if (!filename[file][0]) return 0;
    f = fopen(filename[file], "r");
    length = fread(&_EDITRAM_LAST__, 1, (char*)0x8000 - &_EDITRAM_LAST__, f);
    fclose(f);
    return length;
}

static char write_onefiler_anims(FILE* fout) {
    /* It would be possible to let the first heap start from player end,
     * but it seems like there are problems in magic area around 3FFx...
     */
#define HEAP_COUNT 3
    unsigned int heap_start[3] = { 
        (unsigned int)HEAP_START,  // RAM end - 0x8000 
        0xc000u,  // - 0xd000 
        0xe000u  // - 0xffff 
    };
    static const unsigned int heap_end[3] = { 0x8000u, 0xd000u, 0xffffu };
    unsigned char file_it;
    for (file_it = 0; file_it < FILE_COUNT; ++file_it) {
        const unsigned int file_length = get_file_length(file_it);
        unsigned char heap_it;
        unsigned char alloc_failed = 1;
        if (!file_length) {
            continue;
        }
        for (heap_it = 0; heap_it < sizeof(heap_start) / sizeof(*heap_start); ++heap_it) {
            if (heap_end[heap_it] - heap_start[heap_it] >= file_length + 4) {
                unsigned int addr = (int)heap_start[heap_it];
                addr += 4;  // Include header.
                // Writes address.
                fputc(addr >> 8, fout);
                fputc(addr & 0xffu, fout);
                // Writes size.
                fputc(file_length >> 8, fout);
                fputc(file_length & 0xff, fout);
                heap_start[heap_it] += file_length;
                alloc_failed = 0;
                fwrite(&_EDITRAM_LAST__, file_length, 1, fout);
                break;
            }
        }
        if (alloc_failed) return 0;
    }
    fputc(0, fout);
    return 1;
}

static void save_onefiler() {
    FILE* f;
    _filetype = 'p';  // .prg
    f = prompt_open("demo", "w");
    if (f == NULL) {
        return;
    }
    // Writes load address.
    fputc(1, f);
    fputc(8, f);
    // Saves player program code.
    fwrite((char*)0x801, (unsigned int)HEAP_START - 0x801, 1, f);
    if (HEAP_START < &_RAM_LAST__) {
        // The heap must start after player code ends.
        while (1) {
            ++*(char*)0xd020u;  // Don't let this go unnoticed!
        }
    }
    if (!write_onefiler_anims(f)) {
        textcolor(COLOR_RED);
        puts("out of mem");
        cgetc();
    }
    if (EOF == fclose(f)) {
        textcolor(COLOR_RED);
        puts("disk full?");
        cgetc();
    }
    _filetype = 'u';  // Switch back to .usr
}

static char handle_key(unsigned char key) {
    switch (key) {
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
            save_movie();
            break;
        case CH_F3: prompt_music(); break;
        case CH_F5:
            save_onefiler();
            show_screen();
            break;
        case CH_STOP:
            load_selected_anim(1);
            skip_music_frames();
            init_play();
            play_anim(movie.speed[selected_file], 1);
            wait_anim(movie.frames[selected_file] * movie.speed[selected_file]);
            exit_play();
            show_screen();
            break;
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
            load_selected_anim(0);
            break;
        }
    }
}
