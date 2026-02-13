// Keller-Segel system simulation //

#include "hardware/structs/rosc.h"
#include "ST7735_TFT.hpp"

#define WIDTH   80
#define HEIGHT  160
#define AGENTS  250

float diffuseRate = 0.1f;
float evaporateRate = 0.85f;
float attractStrength = 1.3f;
float noiseStrength = 0.1f;
float inertia = 0.85f;

ST7735_TFT tft;

float chem[WIDTH][HEIGHT];
float nextChem[WIDTH][HEIGHT];

struct Cell {
    float x, y, vx, vy;
};

Cell cells[AGENTS];

static inline void seed_random_from_rosc() {

  uint32_t r = 0;
  volatile uint32_t *rnd_reg = (uint32_t *)(ROSC_BASE + ROSC_RANDOMBIT_OFFSET);
  for (int k = 0; k < 32; k++) r = (r << 1) | ((*rnd_reg) & 1);
  srand(r);

}

void setup() {

  seed_random_from_rosc();
    
  tft.TFTInitSPIType(62500, spi1);
  tft.TFTSetupGPIO(12, 8, 9, 10, 11);
  tft.TFTInitScreenSize(26, 1, WIDTH, HEIGHT);
  tft.TFTInitPCBType(TFT_PCBtype_e::TFT_ST7735S_Black);
  tft.TFTchangeInvertMode(true);
  tft.TFTfillScreen(ST7735_BLACK);

  for(int x=0; x<WIDTH; x++) {
    for(int y=0; y<HEIGHT; y++) chem[x][y] = 0.0f;
  }

  for (int i = 0; i < AGENTS; i++) {
    cells[i] = {(float)(rand() % WIDTH), (float)(rand() % HEIGHT), 0, 0};
  }

}

void loop() {

  for (int x = 1; x < WIDTH - 1; x++) {
    for (int y = 1; y < HEIGHT - 1; y++) {
      float sum = chem[x+1][y] + chem[x-1][y] + chem[x][y+1] + chem[x][y-1];
      nextChem[x][y] = (chem[x][y] + sum * diffuseRate) / (1.0f + 4.0f * diffuseRate);
      nextChem[x][y] *= evaporateRate;
      }
  }

  for(int x=0; x<WIDTH; x++) {
    for(int y=0; y<HEIGHT; y++) chem[x][y] = nextChem[x][y];
  }

  for (int i = 0; i < AGENTS; i++) {
    Cell &c = cells[i];

    float maxC = -1.0f;
    float dirX = 0, dirY = 0;

    for (int ox = -1; ox <= 1; ox++) {
      for (int oy = -1; oy <= 1; oy++) {
        int nx = ((int)c.x + ox + WIDTH) % WIDTH;
        int ny = ((int)c.y + oy + HEIGHT) % HEIGHT;
        if (chem[nx][ny] > maxC) {
          maxC = chem[nx][ny];
          dirX = (float)ox;
          dirY = (float)oy;
        }
      }
    }

    c.vx = c.vx * inertia + dirX * attractStrength + (((rand() % 100) / 50.0f - 1.0f) * noiseStrength);
    c.vy = c.vy * inertia + dirY * attractStrength + (((rand() % 100) / 50.0f - 1.0f) * noiseStrength);
        
    c.x = fmod(c.x + c.vx + WIDTH, (float)WIDTH);
    c.y = fmod(c.y + c.vy + HEIGHT, (float)HEIGHT);

    chem[(int)c.x][(int)c.y] += 2.5f;
  }

  for (int y = 0; y < HEIGHT; y++) {
    for (int x = 0; x < WIDTH; x++) {
      float val = chem[x][y];
      uint8_t coll = 200 * val;
      tft.pushColor(tft.Color565(coll, coll/2 , 255-coll));
    }
  }

}