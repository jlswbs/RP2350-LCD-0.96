// 2D Multi-Scale Turing patterns memory optimised //

#include "hardware/structs/rosc.h"
#include "ST7735_TFT.hpp"

#define WIDTH   80
#define HEIGHT  160
#define SCR     WIDTH*HEIGHT

#define PIN_SCK    10
#define PIN_MOSI   11
#define PIN_RST    12
#define PIN_CS     9
#define PIN_DC     8

  ST7735_TFT tft;

float randomf(float minf, float maxf) {return minf + (rand()%(1UL << 31))*(maxf - minf) / (1UL << 31);} 
  
  #define MAX_LEVELS 64

int level, i, x, y;
int blurlevels;
float base;
int levels;
float stepScale;
float stepOffset;
float blurFactor;

int radii[MAX_LEVELS];
float stepSizes[MAX_LEVELS];

float grid[SCR];
float temp[SCR];
uint8_t bestInfo[SCR];
uint8_t bestVarLow[SCR];

float activator[SCR];
float inhibitor[SCR];

#define GET_BEST_LEVEL(idx) (bestInfo[idx] & 0x3F)
#define GET_DIRECTION(idx) ((bestInfo[idx] & 0x40) != 0)
#define GET_VAR_HIGH(idx) ((bestInfo[idx] & 0x80) != 0)

static inline uint16_t get_variation(int idx) {
  return ((uint16_t)GET_VAR_HIGH(idx) << 8) | bestVarLow[idx];
}

static inline void set_variation(int idx, uint16_t var) {
  bestInfo[idx] = (bestInfo[idx] & 0x7F) | ((var & 0x100) ? 0x80 : 0);
  bestVarLow[idx] = var & 0xFF;
}
    
void rndrule() {

  base = randomf(1.5f, 1.9f);
  levels = (int)(logf(fmaxf(WIDTH, HEIGHT)) / logf(base)) - 1;
  if (levels > MAX_LEVELS) levels = MAX_LEVELS;
  if (levels < 4) levels = 4;

  stepScale = randomf(0.03f, 0.08f) * (8.0f / levels);
  stepOffset = randomf(0.005f, 0.02f);
  blurFactor = randomf(0.7f, 0.9f);
  blurlevels = (int)fmaxf(0, (levels + 1) * blurFactor - 0.5f);

  for (int i = 0; i < levels; i++) {
    int maxRadius = fminf(WIDTH, HEIGHT) / 3;
    radii[i] = fminf((int)powf(base, i), maxRadius);
    stepSizes[i] = logf(radii[i]) * stepScale + stepOffset;
  }

  for (i = 0; i < SCR; i++) grid[i] = randomf(-1.0f, 1.0f);

}

static inline uint16_t better_gray(float v) {

    v = v * 1.5f;
    if (v > 1.0f) v = 1.0f;
    if (v < -1.0f) v = -1.0f;
    
    float sign = v < 0 ? -1.0f : 1.0f;
    v = sign * powf(fabsf(v), 1.0f/2.2f);
    
    int val_int = (int)((v + 1.0f) * 127.5f);
    if (val_int < 0) val_int = 0;
    if (val_int > 255) val_int = 255;
    
    uint8_t val = val_int;
    return ((val >> 3) << 11) | ((val >> 2) << 5) | (val >> 3);
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

  memcpy(activator, grid, 4*SCR);

  for (level = 0; level < levels - 1; level++) {
    int radius = radii[level];

    if (level <= blurlevels) {
      for (y = 0; y < HEIGHT; y++) {
        for (x = 0; x < WIDTH; x++) {
          int t = y * WIDTH + x;
          if (y == 0 && x == 0) {
            temp[t] = activator[t];
          } else if (y == 0) {
            temp[t] = temp[t - 1] + activator[t];
          } else if (x == 0) {
            temp[t] = temp[t - WIDTH] + activator[t];
          } else {
            temp[t] = temp[t - 1] + temp[t - WIDTH] 
                    - temp[t - WIDTH - 1] + activator[t];
          }
        }
      }
      
      for (y = 0; y < HEIGHT; y++) {
        int miny = (y - radius > 0) ? y - radius : 0;
        int maxy = (y + radius < HEIGHT - 1) ? y + radius : HEIGHT - 1;
        
        for (x = 0; x < WIDTH; x++) {
          int minx = (x - radius > 0) ? x - radius : 0;
          int maxx = (x + radius < WIDTH - 1) ? x + radius : WIDTH - 1;
          
          int area = (maxx - minx + 1) * (maxy - miny + 1);
          int t = y * WIDTH + x;
          
          float sum = 0;
          if (minx > 0 && miny > 0) {
            int nw = (miny - 1) * WIDTH + (minx - 1);
            int ne = (miny - 1) * WIDTH + maxx;
            int sw = maxy * WIDTH + (minx - 1);
            int se = maxy * WIDTH + maxx;
            sum = temp[se] - temp[sw] - temp[ne] + temp[nw];
          } else if (miny > 0) {
            int ne = (miny - 1) * WIDTH + maxx;
            int se = maxy * WIDTH + maxx;
            sum = temp[se] - temp[ne];
          } else if (minx > 0) {
            int sw = maxy * WIDTH + (minx - 1);
            int se = maxy * WIDTH + maxx;
            sum = temp[se] - temp[sw];
          } else {
            int se = maxy * WIDTH + maxx;
            sum = temp[se];
          }
          
          inhibitor[t] = sum / area;
        }
      }
    } else {
      memcpy(inhibitor, activator, 4*SCR);
    }

    for (i = 0; i < SCR; i++) {
      float diff = activator[i] - inhibitor[i];
      float var_f = (diff > 0) ? diff : -diff;
      
      uint16_t var = (uint16_t)(var_f * 255.0f);
      if (var > 511) var = 511;
      
      if (level == 0 || var < get_variation(i)) {
        set_variation(i, var);
        bestInfo[i] = (bestInfo[i] & 0xC0) | (level & 0x3F);
        bestInfo[i] = (bestInfo[i] & ~0x40) | ((diff > 0) ? 0x40 : 0);
      }
    }

    if (level == 0) {
      memcpy(activator, inhibitor, 4*SCR);
    } else {
      for (i = 0; i < SCR; i++) {
        float tmp = activator[i];
        activator[i] = inhibitor[i];
        inhibitor[i] = tmp;
      }
    }
  }

  float smallest = MAXFLOAT;
  float largest = -MAXFLOAT;

  for (i = 0; i < SCR; i++) {
    int bestLvl = GET_BEST_LEVEL(i);
    float curStep = stepSizes[bestLvl];
    if (GET_DIRECTION(i)) {
      grid[i] += curStep;
    } else {
      grid[i] -= curStep;
    }
    if (grid[i] < smallest) smallest = grid[i];
    if (grid[i] > largest) largest = grid[i];
  }

  float range = (largest - smallest) / 2.0f;

  for (i = 0; i < SCR; i++) {
    grid[i] = ((grid[i] - smallest) / range) - 1.0f;
    uint16_t image = better_gray(grid[i]);
    tft.pushColor(image);
  }

}