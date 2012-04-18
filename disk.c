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

#include <c64.h>
#include <conio.h>
#include <stdio.h>
#include <string.h>

#pragma codeseg("EDITCODE")

char prompt_path[FILENAME_LENGTH];
FILE* prompt_open(const char* prompt, char write) {
    clrscr();
    gotoxy(0, 0);
    textcolor(COLOR_YELLOW);
    for (;;) {
        FILE* f;
        cputc('\n');
        cputs(prompt);
        cputc('>');
        gets(prompt_path);
        if (!*prompt_path) return 0;
        _filetype = strcmp(prompt, "demo") ? 'u' : 'p';
        if (f = fopen(prompt_path, write ? "w" : "r")) {
            return f;
        }
        cputs("err");
        gotox(0);
    }
}

