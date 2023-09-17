#include <inttypes.h>
#include <pigpio.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

#include "ft8_lib/ft8/constants.h"
#include "ft8_lib/ft8/encode.h"
#include "ft8_lib/ft8/pack.h"
#include "si5351a.h"

void generate_ft8_tones(const char *msg, uint8_t *buffer) {
    // First, pack the text data into binary message
    uint8_t packed[FTX_LDPC_K_BYTES];
    int rc = pack77(msg, packed);
    if (rc < 0) {
        printf("Cannot parse message!\n");
        printf("RC = %d\n", rc);
        si5351aCleanup(-2);
    }

    ft8_encode(packed, buffer);
}

int main(int argc, char **argv) {
    uint8_t symbols[80] = {0};

    si5351aInitialize();

    generate_ft8_tones("CQ VE3XEU FN25", symbols);

    // 1225Hz above the dial frequency for 20m FT8
    const int base = 14074000 + 1225;

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
    for (;;) {
        gettimeofday(&tv, &tz);
        if ((tv.tv_sec % 30) == 0) {
            // Transmit in even numbered periods.
            write(1, "X", 1);
            break;
        }
        write(1, ".", 1);
        usleep(500000);
    }
    usleep(500000);

    si5351aRelayOn();
    for (int i = 0; i < 79; i++) {
        long f = (base + ((symbols[i] + 1) * 6.25)) * 8;
        si5351aSetFrequency(f);
        usleep(150000);
    }
    si5351aCleanup(0);
}
