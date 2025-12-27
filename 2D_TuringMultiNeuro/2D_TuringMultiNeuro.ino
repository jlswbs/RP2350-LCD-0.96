// 2D Multi-scale Turing patterns neuronal adaptation // 

#include "hardware/structs/rosc.h"
#include "ST7735_TFT.hpp"

#define WIDTH   80
#define HEIGHT  160
#define SCR     (WIDTH*HEIGHT)

#define PIN_SCK    10
#define PIN_MOSI   11
#define PIN_RST    12
#define PIN_CS     9
#define PIN_DC     8

ST7735_TFT tft;

float randomf(float minf, float maxf) { return minf + (rand() / (float)RAND_MAX) * (maxf - minf); }

int level, radius, i, x, y;
int blurlevels;
float base;
int levels;
float stepScale;
float stepOffset;
float blurFactor;

int radii[WIDTH];
float stepSizes[WIDTH];
float grid[SCR];
float blurBuffer[SCR];
float bestVariation[SCR];
int bestLevel[SCR];
bool direction[SCR];
float activator[SCR];
float inhibitor[SCR];
float swap[SCR];
float neuronDir[SCR];

#define NEURON_INERTIA  0.97f
#define NEURON_DRIVE    0.015f
#define NEURON_NOISE    0.003f

void rndrule() {

  base = randomf(1.5f, 1.9f);
  stepScale = randomf(0.02f, 0.06f);
  stepOffset = randomf(0.01f, 0.03f);
  blurFactor = randomf(0.7f, 0.9f);

  levels = (int)(log(fmaxf(WIDTH, HEIGHT)) / logf(base)) - 1;
  blurlevels = (int)((levels + 1) * blurFactor);

  for (int i = 0; i < levels; i++) {
    int maxRadius = fminf(WIDTH, HEIGHT) / 3;
    radii[i] = fminf((int)powf(base, i), maxRadius);
    stepSizes[i] = logf(radii[i]) * stepScale + stepOffset;
  }

  for (i = 0; i < SCR; i++) {
    grid[i] = randomf(-1.0f, 1.0f);
    neuronDir[i] = randomf(-0.05f, 0.05f);
    bestVariation[i] = MAXFLOAT;
  }

}

static inline void seed_random_from_rosc(){

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

}

void setup() {

  seed_random_from_rosc();

  tft.TFTInitSPIType(62500, spi1);
  tft.TFTSetupGPIO(PIN_RST, PIN_DC, PIN_CS, PIN_SCK, PIN_MOSI);
  tft.TFTInitScreenSize(26, 1, WIDTH, HEIGHT);
  tft.TFTInitPCBType(TFT_PCBtype_e::TFT_ST7735S_Black);
  tft.TFTchangeInvertMode(true);
  tft.TFTfillScreen(ST7735_BLACK);

  rndrule();

}

void loop() {

  memcpy(activator, grid, sizeof(grid));

  for (level = 0; level < levels - 1; level++) {

    int radius = radii[level];

    if (level <= blurlevels) {

      for (y = 0; y < HEIGHT; y++) {
        for (x = 0; x < WIDTH; x++) {
          int t = y * WIDTH + x;
          if (y == 0 && x == 0)
            blurBuffer[t] = activator[t];
          else if (y == 0)
            blurBuffer[t] = blurBuffer[t - 1] + activator[t];
          else if (x == 0)
            blurBuffer[t] = blurBuffer[t - WIDTH] + activator[t];
          else
            blurBuffer[t] = blurBuffer[t - 1] + blurBuffer[t - WIDTH]
                          - blurBuffer[t - WIDTH - 1] + activator[t];
        }
      }
    }

    for (y = 0; y < HEIGHT; y++) {
      for (x = 0; x < WIDTH; x++) {

        int minx = max(0, x - radius);
        int maxx = min(WIDTH - 1, x + radius);
        int miny = max(0, y - radius);
        int maxy = min(HEIGHT - 1, y + radius);

        int area = (maxx - minx + 1) * (maxy - miny + 1);

        int nw = miny * WIDTH + minx;
        int ne = miny * WIDTH + maxx;
        int sw = maxy * WIDTH + minx;
        int se = maxy * WIDTH + maxx;

        int t = y * WIDTH + x;
        inhibitor[t] = (blurBuffer[se] - blurBuffer[sw]
                      - blurBuffer[ne] + blurBuffer[nw]) / area;
      }
    }

    for (i = 0; i < SCR; i++) {
      float v = fabsf(activator[i] - inhibitor[i]);
      if (level == 0 || v < bestVariation[i]) {
        bestVariation[i] = v;
        bestLevel[i] = level;
        direction[i] = activator[i] > inhibitor[i];
      }
    }

    memcpy(activator, inhibitor, sizeof(grid));
  }

  float smallest = MAXFLOAT;
  float largest = -MAXFLOAT;

  for (i = 0; i < SCR; i++) {

    float target = direction[i] ? 1.0f : -1.0f;
    float step = stepSizes[bestLevel[i]] * NEURON_DRIVE;
    float force = step * target;

    neuronDir[i] = neuronDir[i] * NEURON_INERTIA + target * step + randomf(-NEURON_NOISE, NEURON_NOISE);
    grid[i] += neuronDir[i];

    smallest = min(smallest, grid[i]);
    largest  = max(largest, grid[i]);
  
  }

  float range = (largest - smallest) * 0.5f;

  for (i = 0; i < SCR; i++) {
    grid[i] = ((grid[i] - smallest) / range) - 1.0f;
    uint8_t image = (128 + (127.0f * grid[i]));
    tft.pushColor(tft.Color565(image, image, image));
  }

}