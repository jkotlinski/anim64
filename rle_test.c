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

#include "rle.h"

#include <assert.h>
#include <conio.h>
#include <stdio.h>
#include <string.h>

#define UNPACKED (char*)0x6000u
#define UNPACKED2 (char*)0x8000u
#define PACKED (char*)0xa000u

void test_packunpack() {
    unsigned int packsize = pack(UNPACKED, PACKED, 0x2000);
    unsigned int unpacksize = unpack(PACKED, UNPACKED2, packsize);
    printf("pack %#x %#x %#x => %#x\n", UNPACKED, PACKED, 0x2000, packsize);
    printf("unpack %#x %#x %#x => %#x\n", PACKED, UNPACKED2, packsize, unpacksize);
    assert(unpacksize == 0x2000);
    assert(!memcmp(UNPACKED, UNPACKED2, 0x2000));
}

int main() {
    clrscr();
    textcolor(1);
    test_packunpack();
    while(1);
    return 0;
}

