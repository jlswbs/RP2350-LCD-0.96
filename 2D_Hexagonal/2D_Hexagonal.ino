// 2D Hexagonal cellular automata //

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

#define ITER    HEIGHT/2

  int8_t p[WIDTH][HEIGHT];
  int8_t v[WIDTH][HEIGHT];
  uint16_t image;
  int cnt;

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

void rndrule(){

  for (int x = 0; x < WIDTH; x++) {
    for (int y = 0; y < HEIGHT; y++) {
      p[x][y] = 1;
      v[x][y] = 1;
    }
  }

  p[WIDTH/2][HEIGHT/2] = -1;

}

void setup(){

  seed_random_from_rosc();

  tft.TFTInitSPIType(62500, spi1);
	tft.TFTSetupGPIO(PIN_RST, PIN_DC, PIN_CS, PIN_SCK, PIN_MOSI);
  tft.TFTInitScreenSize(26, 1, WIDTH, HEIGHT);
	tft.TFTInitPCBType(TFT_PCBtype_e::TFT_ST7735S_Black);
  tft.TFTchangeInvertMode(true);
	tft.TFTfillScreen(ST7735_BLACK);

  rndrule();
  
}

void loop(){
  
  for (int x = 1; x < WIDTH-1; x++) {
    for (int y = 1; y < HEIGHT-1; y+=2) {
      v[x][y] *= p[x][y] * p[x-1][y] * p[x+1][y] * p[x][y-1] * p[x][y+1] * p[x+1][y-1] * p[x+1][y+1];
    }
    for (int y = 2; y < HEIGHT-1; y+=2) {
      v[x][y] *= p[x][y] * p[x-1][y] * p[x+1][y] * p[x][y-1] * p[x][y+1] * p[x-1][y+1] * p[x-1][y-1];
    }
  }

  for (int y = 0; y < HEIGHT; y++) {

    for (int x = 0; x < WIDTH; x++) {

      p[x][y] *= v[x][y];
      if (p[x][y] == -1) image = ST7735_WHITE;
      else image = ST7735_BLACK;
      tft.pushColor(image);

    }

  }

  if (cnt == ITER) {

    rndrule();
    cnt = 0;

  }

  cnt++;

}