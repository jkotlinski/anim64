/** Copyright (c) 2012, Johan Kotlinski

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

#include "onefiler.h"

#include "movie.h"

extern volatile unsigned char caught_irqs;

void play_movie_if_onefiler() {
    if (!movie.speed[0]) {
        return;
    }
    for (;;) ++*(char*)0xd020;
    /*
    unsigned int wait_duration = 0;
    unsigned char file_it = 0;
    unsigned char alt_screen = 0;
    init_music();
    init_play();
    for (;;) {
        // unpack_anim(file_it, alt_screen);
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
    */
}

