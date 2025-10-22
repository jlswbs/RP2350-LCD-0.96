// 8x 1D Wolfram cellular automata - perceptron rules //

#include "hardware/structs/rosc.h"
#include "ST7735_TFT.hpp"

#define WIDTH   160
#define HEIGHT  80
#define SCR     (WIDTH * HEIGHT)

#define PIN_SCK    10
#define PIN_MOSI   11
#define PIN_RST    12
#define PIN_CS     9
#define PIN_DC     8

ST7735_TFT tft;

uint16_t framebuffer[SCR];
uint8_t state[WIDTH]; 
uint8_t newstate[WIDTH];

bool rules[8] = {0, 1, 1, 0, 1, 1, 1, 0};

uint16_t colors[8];

float weights[8];
float bias;

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
    tft.TFTsetRotation(TFT_Degrees_270);
    tft.TFTfillScreen(ST7735_BLACK);

    for (int layer = 0; layer < 8; layer++) colors[layer] = rand() & 0xFFFF;

    for (int i = 0; i < 8; i++) {
        weights[i] = ((float)rand() / RAND_MAX) * 2 - 1;
    }
    bias = ((float)rand() / RAND_MAX) * 2 - 1;

    for (int i = 0; i < WIDTH; i++) {
        state[i] = 0;
        for (int layer = 0; layer < 8; layer++) {
            if ((rand() % 100) < 15) {
                state[i] |= (1 << layer);
            }
        }
    }

}

void loop() {

    memmove(framebuffer, framebuffer + WIDTH, (SCR - WIDTH) * sizeof(uint16_t));

    for (int x = 0; x < WIDTH; x++) {

        uint8_t left = state[(x - 1 + WIDTH) % WIDTH];
        uint8_t center = state[x];
        uint8_t right = state[(x + 1) % WIDTH];

        newstate[x] = 0;

        float sum = 0.0f;
        for (int i = 0; i < 8; i++) {
            sum += weights[i] * ((center >> i) & 1);
        }
        sum += bias;
        int interaction = (sum > 0) ? 1 : 0;

        for (int layer = 0; layer < 8; layer++) {

            int l = (left >> layer) & 1;
            int c = (center >> layer) & 1;
            int r = (right >> layer) & 1;

            int k = 4 * l + 2 * c + r;
            k = (k + interaction) % 8;

            if (rules[k]) {
                newstate[x] |= (1 << layer);
            }
        }

        uint16_t pixel_color = ST7735_BLACK;
        for (int layer = 0; layer < 8; layer++) {
            if (newstate[x] & (1 << layer)) {
                pixel_color = colors[layer];
            }
        }

        framebuffer[(HEIGHT - 1) * WIDTH + x] = pixel_color;
    }

    memcpy(state, newstate, sizeof(state));

    tft.TFTsetAddrWindow(0, 0, WIDTH - 1, HEIGHT - 1);
    for (int i = 0; i < SCR; i++) tft.pushColor(framebuffer[i]);

}