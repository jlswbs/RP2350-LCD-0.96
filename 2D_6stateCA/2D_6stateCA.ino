// 2D Cellular automata 6 states //

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

  uint8_t grid[WIDTH][HEIGHT];
  uint8_t newGrid[WIDTH][HEIGHT];
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

  for (int x = 0; x < WIDTH; x++) {

    for (int y = 0; y < HEIGHT; y++) grid[x][y] = random(6);

  }

}

void updateGrid() {

  for (int y = 0; y < HEIGHT; y++) {
  
    for (int x = 0; x < WIDTH; x++) {

      int current_state = grid[x][y];
      int neighborCounts[6] = {0, 0, 0, 0, 0, 0};

      for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
          if (dx == 0 && dy == 0) continue;
          int nx = (x + dx + WIDTH) % WIDTH;
          int ny = (y + dy + HEIGHT) % HEIGHT;
          neighborCounts[grid[nx][ny]]++;
        }
      }

      int next_state = (current_state + 1) % 6;
      int prev_state = (current_state - 1 + 6) % 6;
      if (neighborCounts[next_state] >= 3) {
        newGrid[x][y] = next_state;
      } else if (neighborCounts[current_state] < 2) {
        newGrid[x][y] = prev_state;
      } else {
        newGrid[x][y] = current_state;
      }
    }

  }

  for (int y = 0; y < HEIGHT; y++) {

    for (int x = 0; x < WIDTH; x++) grid[x][y] = newGrid[x][y];

  }

}

int countNeighbors(int x, int y) {

  int count = 0;

  for (int dx = -1; dx <= 1; dx++) {

    for (int dy = -1; dy <= 1; dy++) {

      if (dx == 0 && dy == 0) continue;
      int nx = (x + dx + WIDTH) % WIDTH;
      int ny = (y + dy + HEIGHT) % HEIGHT;
      if (grid[nx][ny] >= 1) count++;

    }

  }

  return count;

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

  updateGrid();

  for (int y = 0; y < HEIGHT; y++) {

    for (int x = 0; x < WIDTH; x++) {

      if (grid[x][y] == 0) color = ST7735_BLACK;
      if (grid[x][y] == 1) color = ST7735_RED;
      if (grid[x][y] == 2) color = ST7735_GREEN;
      if (grid[x][y] == 3) color = ST7735_BLUE;
      if (grid[x][y] == 4) color = ST7735_YELLOW;
      if (grid[x][y] == 5) color = ST7735_WHITE;

      tft.pushColor(color);

    }
  
  }

}