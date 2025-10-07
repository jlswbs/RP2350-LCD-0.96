// 2D Voronoi distribution (cell noise) //
// -O3 compiler optimization required //

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

#define PARTICLES 16

  float mindist = 0;
  unsigned int x[PARTICLES];
  unsigned int y[PARTICLES];
  unsigned int dx[PARTICLES];
  unsigned int dy[PARTICLES];

float distance(int x1, int y1, int x2, int y2) { return sqrtf(powf(x2 - x1, 2.0f) + powf(y2 - y1, 2.0f)); }

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

void rndseed(){

  for (int i=0; i<PARTICLES; i++) {
    
    x[i] = rand()%WIDTH;
    y[i] = rand()%HEIGHT;
    dx[i] = rand()%5;
    dy[i] = rand()%5;
    
  }
  
}

void setup(){

  seed_random_from_rosc();

  tft.TFTInitSPIType(62500, spi1);
	tft.TFTSetupGPIO(PIN_RST, PIN_DC, PIN_CS, PIN_SCK, PIN_MOSI);
  tft.TFTInitScreenSize(26, 1, WIDTH, HEIGHT);
	tft.TFTInitPCBType(TFT_PCBtype_e::TFT_ST7735S_Black);
  tft.TFTchangeInvertMode(true);
	tft.TFTfillScreen(ST7735_BLACK);
  
  rndseed();
  
}

void loop(){

  for (int j=0; j<HEIGHT; j++) {
    
    for (int i=0; i<WIDTH; i++) {
      
      mindist = PI * WIDTH;
      
      for (int p=0; p<PARTICLES; p++) {

        if (distance(x[p], y[p], i, j) < mindist) mindist = distance(x[p], y[p], i, j);
        if (distance(x[p]+WIDTH, y[p], i, j) < mindist) mindist = distance(x[p]+WIDTH, y[p], i, j);
        if (distance(x[p]-WIDTH, y[p], i, j) < mindist) mindist = distance(x[p]-WIDTH, y[p], i, j);
        
      }
      
      uint8_t coll = PI * mindist;
      tft.pushColor(tft.Color565(coll, coll, coll));

    }

  }
  
  for (int p=0; p<PARTICLES; p++) {

    x[p] += dx[p];
    y[p] += dy[p];
    x[p] = x[p] % WIDTH;
    y[p] = y[p] % HEIGHT;

  }

}