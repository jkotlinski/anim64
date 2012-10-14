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

#include "disk.h"

#include <stdio.h>
#include <string.h>
#include "cc65/mycbm.h"

#include <conio.h>
#pragma codeseg("EDITCODE")
#pragma rodataseg("EDITCODE")

static void list_files() {
    unsigned char ent[DIRENT_SIZE];
    opendir(1, 8);
    while (!readdir(1, ent)) {
        cputs(ent);
        cputc('\n');
        gotox(0);
    }
    closedir(1);
}

char prompt_path[FILENAME_LENGTH];
unsigned char prompt_open(const char* prompt, char mode, char type) {
    clrscr();
    gotoxy(0, 0);
    textcolor(COLOR_YELLOW);
    for (;;) {
        cputc('\n');
        cputs(prompt);
        cputs(" (*=list)>");
        if (mode == CBM_WRITE) {
            prompt_path[0] = 's';
            prompt_path[1] = ':';
            if (!*gets(prompt_path + 2)) return 0;
            if (prompt_path[2] == '*') {
                list_files();
                continue;
            }
            // Scratch file.
            if (mycbm_open(1, 8, 15, prompt_path)) {
                goto err;
            }
            mycbm_close(1);
            memmove(prompt_path, prompt_path + 2, sizeof(prompt_path) - 2);
        } else {
            if (!*gets(prompt_path)) return 0;
            if (prompt_path[0] == '*') {
                list_files();
                continue;
            }
        }
        strcat(prompt_path, type ? ",p" : ",u");
        if (!mycbm_open(MY_LFN, 8, mode, prompt_path)) return 1;
err:
        cputs("err");
        gotox(0);
    }
}

