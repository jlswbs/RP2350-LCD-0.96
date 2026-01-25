// 2D Cyclic cellular automata - defined rules //

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

const char rules[] = "RLLLLRRRLLLR";

#define ITER     120
#define N_COLORS (sizeof(rules) - 1)
#define IDX(x, y) ((y) * WIDTH + (x))

uint8_t grid[SCR];

struct Ant {
  int x, y;
  int dir;
} ant;

uint16_t cell_to_color(uint8_t state) {

  float hue = state * (360.0f / N_COLORS);
  float s = 0.8f;
  float v = 0.8f;
    
  int i = (int)(hue / 60.0f) % 6;
  float f = hue / 60.0f - i;
  float p = v * (1.0f - s);
  float q = v * (1.0f - s * f);
  float t = v * (1.0f - s * (1.0f - f));
    
  float r_f, g_f, b_f;
  switch (i) {
    case 0: r_f = v; g_f = t; b_f = p; break;
    case 1: r_f = q; g_f = v; b_f = p; break;
    case 2: r_f = p; g_f = v; b_f = t; break;
    case 3: r_f = p; g_f = q; b_f = v; break;
    case 4: r_f = t; g_f = p; b_f = v; break;
    default: r_f = v; g_f = p; b_f = q; break;
  }
    
  int r = (int)(r_f * 31);
  int g = (int)(g_f * 63);
  int b = (int)(b_f * 31);
    
  return (r << 11) | (g << 5) | b;

}

void step_ant() {

  int idx = IDX(ant.x, ant.y);
  uint8_t cell_state = grid[idx];
  
  if (rules[cell_state] == 'L') {
    ant.dir = (ant.dir + 3) % 4;
  } else if (rules[cell_state] == 'R') {
    ant.dir = (ant.dir + 1) % 4;
  }
  
  grid[idx] = (cell_state + 1) % N_COLORS;

  switch (ant.dir) {
    case 0: ant.y = (ant.y - 1 + HEIGHT) % HEIGHT; break;
    case 1: ant.x = (ant.x + 1) % WIDTH; break;
    case 2: ant.y = (ant.y + 1) % HEIGHT; break;
    case 3: ant.x = (ant.x - 1 + WIDTH) % WIDTH; break;
  }

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

}

void setup() {

  seed_random_from_rosc();
    
  tft.TFTInitSPIType(62500, spi1);
  tft.TFTSetupGPIO(PIN_RST, PIN_DC, PIN_CS, PIN_SCK, PIN_MOSI);
  tft.TFTInitScreenSize(26, 1, WIDTH, HEIGHT);
  tft.TFTInitPCBType(TFT_PCBtype_e::TFT_ST7735S_Black);
  tft.TFTchangeInvertMode(true);
  tft.TFTfillScreen(ST7735_BLACK);
  
  ant.x = rand() % WIDTH;
  ant.y = rand() % HEIGHT;
  ant.dir = rand() % 4;

}

void loop() {

  for (int i = 0; i < ITER; i++) step_ant();
  
  for (int i = 0; i < SCR; i++) tft.pushColor(cell_to_color(grid[i]));

}