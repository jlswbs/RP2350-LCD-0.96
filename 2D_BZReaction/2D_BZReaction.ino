// 2D Belousov-Zhabotinsky reaction //

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

  float adjust = 1.2f;
  float a [WIDTH][HEIGHT][2];
  float b [WIDTH][HEIGHT][2];
  float c [WIDTH][HEIGHT][2];
  int p = 0, q = 1;

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

void rndrule(){

  adjust = randomf(0.75f, 1.35f);

  for (int y = 0; y < HEIGHT; y++) {

    for (int x = 0; x < WIDTH; x++) {

      a[x][y][0] = randomf(0.0f, 1.0f);
      b[x][y][0] = randomf(0.0f, 1.0f);
      c[x][y][0] = randomf(0.0f, 1.0f);

    }
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

  rndrule();
  
}

void loop(){

  for (int y = 0; y < HEIGHT; y++) {

    for (int x = 0; x < WIDTH; x++) {

      float c_a = 0.0f;
      float c_b = 0.0f;
      float c_c = 0.0f;

      for (int i = x - 1; i <= x+1; i++) {

        for (int j = y - 1; j <= y+1; j++) {

          c_a += a[(i+WIDTH)%WIDTH][(j+HEIGHT)%HEIGHT][p];
          c_b += b[(i+WIDTH)%WIDTH][(j+HEIGHT)%HEIGHT][p];
          c_c += c[(i+WIDTH)%WIDTH][(j+HEIGHT)%HEIGHT][p];

        }
      }

      c_a /= 9.0f;
      c_b /= 9.0f;
      c_c /= 9.0f;

      a[x][y][q] = constrain(c_a + c_a * (adjust * c_b - c_c), 0.0f, 1.0f);
      b[x][y][q] = constrain(c_b + c_b * (c_c - adjust * c_a), 0.0f, 1.0f);
      c[x][y][q] = constrain(c_c + c_c * (c_a - c_b), 0.0f, 1.0f);
      
      uint16_t coll1 = (255.0f * a[x][y][q]);
      uint16_t coll2 = (255.0f * b[x][y][q]);
      uint16_t coll3 = (255.0f * c[x][y][q]);

      tft.pushColor(tft.Color565(coll1, coll2, coll3));

    }

  }
  
  if (p == 0) { p = 1; q = 0; } else { p = 0; q = 1; }

}