// 2D Cellular automata 2 states 2 layers //

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

  uint8_t grid1[WIDTH][HEIGHT];
  uint8_t grid2[WIDTH][HEIGHT];
  uint8_t new_grid1[WIDTH][HEIGHT];
  uint8_t new_grid2[WIDTH][HEIGHT];
  uint16_t color;

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

      grid1[x][y] = random(2);
      grid2[x][y] = random(2);

    }

  }

}

uint8_t count_neighbors(uint8_t x, uint8_t y, uint8_t state, uint8_t grid[WIDTH][HEIGHT]) {

  uint8_t count = 0;

  for (int8_t i = -1; i <= 1; i++) {
    for (int8_t j = -1; j <= 1; j++) {
      if (i == 0 && j == 0) continue;
      uint8_t nx = (x + i + WIDTH) % WIDTH;
      uint8_t ny = (y + j + HEIGHT) % HEIGHT;
      if (grid[nx][ny] == state) count++;
    }
  }

  return count;

}

void update_grid() {

  for (uint8_t y = 0; y < HEIGHT; y++) {

    for (uint8_t x = 0; x < WIDTH; x++) {

      uint8_t current_state1 = grid1[x][y];
      uint8_t current_state2 = grid2[x][y];
      uint8_t neighbors1 = count_neighbors(x, y, 1, grid1);
      uint8_t neighbors2 = count_neighbors(x, y, 1, grid2);

      new_grid1[x][y] = current_state1;
      if (current_state1 == 1) {
        if (neighbors1 < 2 || neighbors1 > 3) new_grid1[x][y] = 0;
      } else {
        if (neighbors1 == 3) new_grid1[x][y] = 1;
      }

      new_grid2[x][y] = current_state2;
      if (current_state2 == 1) {
        if (neighbors2 < 2 || neighbors2 > 3) new_grid2[x][y] = 0;
      } else {
        if (current_state1 == 1 && neighbors2 == 2) new_grid2[x][y] = 1;
        else if (neighbors2 == 3) new_grid2[x][y] = 1;
      }

    }

  }

  for (uint8_t y = 0; y < HEIGHT; y++) {

    for (uint8_t x = 0; x < WIDTH; x++) {

      grid1[x][y] = new_grid1[x][y];
      grid2[x][y] = new_grid2[x][y];

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
  
  rndseed();
  
}

void loop(){

  update_grid();

  for (int y = 0; y < HEIGHT; y++) {

    for (int x = 0; x < WIDTH; x++) {

      if (grid1[x][y] == 0 && grid2[x][y] == 0) color = ST7735_BLACK;
      else if (grid1[x][y] == 0 && grid2[x][y] == 1) color = ST7735_GREEN;
      else if (grid1[x][y] == 1 && grid2[x][y] == 0) color = ST7735_RED;
      else color = ST7735_WHITE;

      tft.pushColor(color);

    }
  
  }

}