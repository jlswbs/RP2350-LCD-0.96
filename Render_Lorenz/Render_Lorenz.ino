// Lorenz chaotic attractor - 2D rendering //

#include "hardware/structs/rosc.h"
#include "ST7735_TFT.hpp"

#define WIDTH   80
#define HEIGHT  160
#define SCR     (WIDTH * HEIGHT)

#define PIN_SCK    10
#define PIN_MOSI   11
#define PIN_RST    12
#define PIN_CS     9
#define PIN_DC     8

ST7735_TFT tft;

float s = 10.0f;
float r = 28.0f;
float b = 8.0f / 3.0f;

float x = 1.0f;
float y = 1.0f;
float z = 1.0f;
float dt = 0.005f;

uint8_t densityBuffer[WIDTH][HEIGHT] = {0};

#define ITERATIONS 100000
#define WARMUP 2000

void lorenz() {

    float nx = x;
    float ny = y;
    float nz = z;
    
    x = nx + dt * (s * (ny - nx));
    y = ny + dt * (nx * (r - nz) - ny);
    z = nz + dt * (nx * ny - b * nz);

}

static inline void seed_random_from_rosc() {

    uint32_t random = 0;
    uint32_t random_bit;
    volatile uint32_t *rnd_reg = (uint32_t *)(ROSC_BASE + ROSC_RANDOMBIT_OFFSET);

    for (int k = 0; k < 32; k++) {
        while (1) {
            random_bit = (*rnd_reg) & 1;
            if (random_bit != ((*rnd_reg) & 1)) break;
        }
        random = (random << 1) | random_bit;
    }
    srand(random);
    randomSeed(random);

}

void setup() {

    seed_random_from_rosc();

    tft.TFTInitSPIType(62500, spi1);
    tft.TFTSetupGPIO(PIN_RST, PIN_DC, PIN_CS, PIN_SCK, PIN_MOSI);
    tft.TFTInitScreenSize(26, 1, WIDTH, HEIGHT);
    tft.TFTInitPCBType(TFT_PCBtype_e::TFT_ST7735S_Black);
    tft.TFTchangeInvertMode(true);
    tft.TFTfillScreen(ST7735_BLACK);

    for (int i = 0; i < ITERATIONS + WARMUP; i++) {

        lorenz();

        if (i >= WARMUP) {

            int px = (int)((50.0f - z) / 50.0f * (WIDTH - 1));
            int py = (int)((x + 20.0f) / 40.0f * (HEIGHT - 1));

            if (px >= 0 && px < WIDTH && py >= 0 && py < HEIGHT) {
                if (densityBuffer[px][py] < 255) {
                    densityBuffer[px][py]++;
                }
            }
        }
    }

    uint8_t maxDensity = 0;
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            if (densityBuffer[x][y] > maxDensity) {
                maxDensity = densityBuffer[x][y];
            }
        }
    }

    for (int y = 0; y < HEIGHT; y++) {

        for (int x = 0; x < WIDTH; x++) {

            uint8_t coll = (maxDensity > 0) ? (uint8_t)(255.0f * powf((float)densityBuffer[x][y] / maxDensity, 0.5f)) : 0;
            tft.pushColor(tft.Color565(coll, coll, coll));
        
        }
    
    }

}

void loop() {

}