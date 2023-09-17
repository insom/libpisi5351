#include <inttypes.h>
#include <pigpio.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

#include "si5351a.h"

int main(int argc, char **argv) {
    si5351aInitialize();
    // VE3XEU FN25 1.995W
    char symbols[] = {
        3, 1, 2, 0, 0, 0, 2, 0, 1, 2, 2, 0, 3, 3, 3, 0, 2, 2, 3, 2, 2, 3, 2, 3,
        3, 3, 3, 0, 2, 2, 2, 2, 0, 2, 3, 0, 2, 1, 2, 3, 0, 0, 0, 2, 0, 0, 1, 0,
        3, 1, 2, 2, 3, 1, 2, 3, 2, 2, 2, 1, 3, 0, 1, 2, 2, 0, 2, 3, 1, 2, 3, 2,
        1, 2, 3, 0, 3, 2, 2, 3, 2, 2, 1, 2, 1, 1, 2, 0, 0, 3, 3, 0, 1, 2, 1, 2,
        0, 0, 1, 0, 0, 0, 2, 2, 1, 2, 2, 1, 2, 2, 1, 3, 1, 0, 3, 1, 2, 0, 1, 1,
        0, 1, 0, 2, 2, 1, 3, 3, 2, 0, 2, 0, 0, 3, 2, 3, 2, 0, 1, 3, 2, 0, 2, 0,
        0, 2, 0, 1, 3, 2, 1, 2, 1, 3, 2, 0, 0, 3, 3, 2, 0, 2};

    // 1700Hz above the dial frequency for 20m FT8
    const int base = (14095600 + 1700);

    if (argc > 1) {
        si5351aSetFrequency(atoi(argv[1]) * 8);
        printf("Transmitting a test tone of %s.\n", argv[1]);
        printf("Press <return> to exit.\n");
        char buf;
        read(0, &buf, 1);
        si5351aCleanup(0);
    }

    struct timeval tv;
    struct timezone tz;
    for (int x = 0; x < 3; x++) {
        for (;;) {
            gettimeofday(&tv, &tz);
            if ((tv.tv_sec % 120) == 1) {
                // Transmit in even numbered minutes.
                write(1, "X", 1);
                break;
            }
            write(1, ".", 1);
            usleep(100000);
        }

        si5351aRelayOn();
        for (int i = 0; i < 162; i++) {
            float f = (base * 8) + ((symbols[i] + 0) * 12);  // was 14
            si5351aSetFrequency(f);
            usleep(680000);  // 110.6 / 162
        }
        si5351aRelayOff();
        si5351aSetFrequency(base - 1000);  // Move down-band but don't disable
                                           // output, to keep PLL in sync
    }
    si5351aCleanup(0);
}
