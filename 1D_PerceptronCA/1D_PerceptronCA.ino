// 1D Perceptron cellular automata like Wolfram //

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

#define RULE 126

uint16_t framebuffer[SCR];
bool state[WIDTH];
bool newstate[WIDTH];

struct Perceptron {
  int8_t w[3];
  float b;
};

Perceptron detectors[8];

void setupPerceptrons() {

  for (int i = 0; i < 8; i++) {
    uint8_t L = (i >> 2) & 1;
    uint8_t C = (i >> 1) & 1;
    uint8_t R = (i >> 0) & 1;

    int k = L + C + R;
    detectors[i].w[0] = (L == 1) ? 1 : -1;
    detectors[i].w[1] = (C == 1) ? 1 : -1;
    detectors[i].w[2] = (R == 1) ? 1 : -1;
    detectors[i].b = -(k - 0.5f);
  }

}

uint8_t perceptronFire(const Perceptron &p, uint8_t L, uint8_t C, uint8_t R) {
  float s = p.w[0]*L + p.w[1]*C + p.w[2]*R + p.b;
  return (s > 0.0f) ? 1 : 0;
}

uint8_t ruleNextCell(uint8_t L, uint8_t C, uint8_t R) {

  uint8_t det[8];
  for (int i = 0; i < 8; i++) {
    det[i] = perceptronFire(detectors[i], L, C, R);
  }

  for (int i = 0; i < 8; i++) {
    if (det[i] && ((RULE >> i) & 1)) {
      return 1;
    }
  }
  return 0;

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
    tft.TFTsetRotation(TFT_Degrees_270);
    tft.TFTfillScreen(ST7735_BLACK);

    setupPerceptrons();

    for (int i = 0; i < WIDTH; i++) state[i] = rand() % 2;

}

void loop() {

    memmove(framebuffer, framebuffer + WIDTH, (SCR - WIDTH) * sizeof(uint16_t));

    for (int x = 0; x < WIDTH; x++) {

        uint8_t L = state[(x - 1 + WIDTH) % WIDTH];
        uint8_t C = state[x];
        uint8_t R = state[(x + 1) % WIDTH];
        newstate[x] = ruleNextCell(L, C, R);

        framebuffer[(HEIGHT - 1) * WIDTH + x] = newstate[x] ? ST7735_WHITE : ST7735_BLACK;

    }
   
    memcpy(state, newstate, sizeof(state));
        
    tft.TFTsetAddrWindow(0, 0, WIDTH - 1, HEIGHT - 1);
    for (int i = 0; i < SCR; i++) tft.pushColor(framebuffer[i]);

}