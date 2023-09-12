#include <stdlib.h>
#include <stdio.h>
#include <pigpio.h>
#include <inttypes.h>
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>
#include "si5351a.h"

#define I2C_WRITE 0x60
#define I2C_READ  0x61
#define I2C_BUS 1

#include "ft8_lib/ft8/pack.h"
#include "ft8_lib/ft8/encode.h"
#include "ft8_lib/ft8/constants.h"

//
// Set up specified PLL with mult, num and denom
// mult is 15..90
// num is 0..1,048,575 (0xFFFFF)
// denom is 0..1,048,575 (0xFFFFF)
//
void setupPLL(int i2c, uint8_t pll, uint8_t mult, uint32_t num, uint32_t denom)
{
    uint32_t P1;					// PLL config register P1
    uint32_t P2;					// PLL config register P2
    uint32_t P3;					// PLL config register P3

    P1 = (uint32_t)(128 * ((float)num / (float)denom));
    P1 = (uint32_t)(128 * (uint32_t)(mult) + P1 - 512);
    P2 = (uint32_t)(128 * ((float)num / (float)denom));
    P2 = (uint32_t)(128 * num - denom * P2);
    P3 = denom;

    i2cWriteByteData(i2c, pll + 0, (P3 & 0x0000FF00) >> 8);
    i2cWriteByteData(i2c, pll + 1, (P3 & 0x000000FF));
    i2cWriteByteData(i2c, pll + 2, (P1 & 0x00030000) >> 16);
    i2cWriteByteData(i2c, pll + 3, (P1 & 0x0000FF00) >> 8);
    i2cWriteByteData(i2c, pll + 4, (P1 & 0x000000FF));
    i2cWriteByteData(i2c, pll + 5, ((P3 & 0x000F0000) >> 12) | ((P2 & 0x000F0000) >> 16));
    i2cWriteByteData(i2c, pll + 6, (P2 & 0x0000FF00) >> 8);
    i2cWriteByteData(i2c, pll + 7, (P2 & 0x000000FF));
}

//
// Set up MultiSynth with integer divider and R divider
// R divider is the bit value which is OR'ed onto the appropriate register, it
// is a #define in si5351a.h
//
void setupMultisynth(int i2c, uint8_t synth, uint32_t divider, uint8_t rDiv)
{
    uint32_t P1;					// Synth config register P1
    uint32_t P2;					// Synth config register P2
    uint32_t P3;					// Synth config register P3

    P1 = 128 * divider - 512;
    P2 = 0;							// P2 = 0, P3 = 1 forces an integer value for the divider
    P3 = 1;

    i2cWriteByteData(i2c, synth + 0,   (P3 & 0x0000FF00) >> 8);
    i2cWriteByteData(i2c, synth + 1,   (P3 & 0x000000FF));
    i2cWriteByteData(i2c, synth + 2,   ((P1 & 0x00030000) >> 16) | rDiv);
    i2cWriteByteData(i2c, synth + 3,   (P1 & 0x0000FF00) >> 8);
    i2cWriteByteData(i2c, synth + 4,   (P1 & 0x000000FF));
    i2cWriteByteData(i2c, synth + 5,   ((P3 & 0x000F0000) >> 12) | ((P2 & 0x000F0000) >> 16));
    i2cWriteByteData(i2c, synth + 6,   (P2 & 0x0000FF00) >> 8);
    i2cWriteByteData(i2c, synth + 7,   (P2 & 0x000000FF));
}

//
// Switches off Si5351a output
// Example: si5351aOutputOff(SI_CLK0_CONTROL);
// will switch off output CLK0
//
void si5351aOutputOff(uint8_t clk)
{
    int i2c = i2cOpen(I2C_BUS, I2C_WRITE, 0);

    i2cWriteByteData(i2c, clk, 0x80);

    i2cClose(i2c);
}

// 
// Set CLK0 output ON and to the specified frequency
// Frequency is in the range 1MHz to 150MHz
// Example: si5351aSetFrequency(10000000);
// will set output CLK0 to 10MHz
//
// This example sets up PLL A and MultiSynth 0 and produces the output on CLK0
//
void si5351aSetFrequency(float frequency)
{
    uint32_t pllFreq;
    uint32_t xtalFreq = XTAL_FREQ;
    uint32_t l;
    float f;
    uint8_t mult;
    uint32_t num;
    uint32_t denom;
    uint32_t divider;

    int i2c = i2cOpen(I2C_BUS, I2C_WRITE, 0);

    divider = 900000000 / frequency;// Calculate the division ratio. 900,000,000 is the maximum internal 
    // PLL frequency: 900MHz
    if (divider % 2) divider--;		// Ensure an even integer division ratio

    pllFreq = divider * frequency;	// Calculate the pllFrequency: the divider * desired output frequency

    mult = pllFreq / xtalFreq;		// Determine the multiplier to get to the required pllFrequency
    l = pllFreq % xtalFreq;			// It has three parts:
    f = l;							// mult is an integer that must be in the range 15..90
    f *= 1048575;					// num and denom are the fractional parts, the numerator and denominator
    f /= xtalFreq;					// each is 20 bits (range 0..1048575)
    num = f;						// the actual multiplier is  mult + num / denom
    denom = 1048575;				// For simplicity we set the denominator to the maximum 1048575

    // Set up PLL A with the calculated multiplication ratio
    setupPLL(i2c, SI_SYNTH_PLL_A, mult, num, denom);
    // Set up MultiSynth divider 0, with the calculated divider. 
    // The final R division stage can divide by a power of two, from 1..128. 
    // reprented by constants SI_R_DIV1 to SI_R_DIV128 (see si5351a.h header file)
    // If you want to output frequencies below 1MHz, you have to use the 
    // final R division stage
    setupMultisynth(i2c, SI_SYNTH_MS_0, divider, SI_R_DIV_1);
    // Reset the PLL. This causes a glitch in the output. For small changes to 
    // the parameters, you don't need to reset the PLL, and there is no glitch
    i2cWriteByteData(i2c, SI_PLL_RESET, 0xA0);	
    // Finally switch on the CLK0 output (0x4F)
    // and set the MultiSynth0 input to be PLL A
    i2cWriteByteData(i2c, SI_CLK0_CONTROL, 0x4F | SI_CLK_SRC_PLL_A);

    i2cClose(i2c);
}

void cleanup(int err) {
    si5351aOutputOff(SI_CLK0_CONTROL);
    gpioTerminate();
    printf("\nExiting.\n");
    exit(err);
}

void generate_ft8_tones(const char *msg, uint8_t *buffer) {
    // First, pack the text data into binary message
    uint8_t packed[FTX_LDPC_K_BYTES];
    int rc = pack77(msg, packed);
    if (rc < 0) {
        printf("Cannot parse message!\n");
        printf("RC = %d\n", rc);
        cleanup(-2);
    }

    ft8_encode(packed, buffer);
}

int main(int argc, char **argv) {
    gpioInitialise();
    // CQ VE3NNE
    uint8_t symbols[] = { 3, 1, 4, 0, 6, 5, 2, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 4, 0,
        2, 6, 0, 0, 0, 3, 3, 0, 6, 0, 3, 6, 3, 3, 5, 6, 4, 3, 1, 4, 0, 6, 5, 2,
        1, 7, 4, 6, 4, 2, 0, 0, 3, 0, 4, 7, 1, 2, 2, 2, 1, 6, 2, 1, 3, 7, 2, 5,
        5, 7, 7, 0, 1, 3, 1, 4, 0, 6, 5, 2 };

    // 1225Hz above the dial frequency for 20m FT8
    const int base = 14074000 + 1225;
    signal(SIGINT, cleanup);

    if(argc > 1) {
        si5351aSetFrequency(atoi(argv[1]));
        printf("Transmitting a test tone of %s.\n", argv[1]);
        printf("Press <return> to exit.\n");
        char buf;
        read(0, &buf, 1);
        cleanup(0);
    }

    struct timeval tv;
    struct timezone tz;
    for(;;) {
        gettimeofday(&tv, &tz);
        if((tv.tv_sec % 30) == 0) {
            // Transmit in even numbered periods.
            write(1, "X", 1);
            break;
        }
        write(1, ".", 1);
        usleep(500000);
    }
    usleep(500000);

    for(int i = 0; i < 79; i++) {
        long f = base + ((symbols[i] + 1) * 6.25);
        si5351aSetFrequency(f);
        usleep(150000);
    }
    cleanup(0);
}
