/*
   This program is provided under the LGPL license ver 2.1
   KM-BASIC for ARM, written by Katsumi.
   http://hp.vector.co.jp/authors/VA016157/
   kmorimatsu@users.sourceforge.jp
   https://github.com/kmorimatsu
*/

#include <stdio.h>
#include "pico/stdlib.h"
#include "api.h"

int main() {
    stdio_init_all();
    while (true) {
        printstr("Hello, KM-BASIC!\n");
        sleep_ms(1000);
    }
    return 0;
}
