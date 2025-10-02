// 2D Amoeba patterns //

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

#define NUMS    64
#define CNUM    32

  float p[WIDTH][HEIGHT];
  float v[WIDTH][HEIGHT];
  float a[WIDTH][HEIGHT];
  uint16_t color[CNUM];

float randomf(float minf, float maxf) {return minf + (rand()%(1UL << 31))*(maxf - minf) / (1UL << 31);}

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

  for (int y = 0; y < HEIGHT; y++) {

    for (int x = 0; x < WIDTH; x++) {

      p[x][y] = 0;
      v[x][y] = 0;
      a[x][y] = 0;
    
    }
  
  }

  for (int i = 0; i < NUMS; i++) v[1+rand()%(WIDTH-2)][1+rand()%(HEIGHT-2)] = randomf(0.0f, 1.0f);
  for (int i = 0; i < CNUM; i++) color[i] = rand();
  
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

  for (int y = 1; y < HEIGHT-1; y++) {

    for (int x = 1; x < WIDTH-1; x++) {

      a[x][y] = (v[x-1][y] + v[x+1][y] + v[x][y-1] + v[x][y+1]) * 0.25f - v[x][y];
    
    }
  
  }
  
  for (int y = 0; y < HEIGHT; y++) {

    for (int x = 0; x < WIDTH; x++) {

      v[x][y] += a[x][y];
      p[x][y] += v[x][y];
      uint8_t coll = CNUM * logf(p[x][y]);
      tft.pushColor(color[coll%CNUM]);

    }
  
  }

}