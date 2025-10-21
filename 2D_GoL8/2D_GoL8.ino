// 8 x 2D Game of Life cellular automata - independent layers //

#include "hardware/structs/rosc.h"
#include "ST7735_TFT.hpp"

#define WIDTH   80
#define HEIGHT  160
#define SCR     (WIDTH * HEIGHT)

#define PIN_SCK    10
#define PIN_MOSI   11
#define PIN_RST    12
#define PIN_CS     9
#define PIN_DC     8

ST7735_TFT tft;

#define NUM_LAYERS 8

uint8_t grid[WIDTH][HEIGHT];
uint8_t new_grid[WIDTH][HEIGHT];

const uint16_t layer_colors[NUM_LAYERS] = {
  ST7735_RED,
  ST7735_GREEN,
  ST7735_BLUE,
  ST7735_YELLOW,
  ST7735_CYAN,
  ST7735_MAGENTA,
  ST7735_WHITE,
  ST7735_ORANGE
};

void set_cell(uint8_t grid[WIDTH][HEIGHT], int x, int y, int layer, bool value) {
  if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT && layer >= 0 && layer < NUM_LAYERS) {
    if (value) {
      grid[x][y] |= (1 << layer);
    } else {
      grid[x][y] &= ~(1 << layer);
    }
  }
}

bool get_cell(const uint8_t grid[WIDTH][HEIGHT], int x, int y, int layer) {
  if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT && layer >= 0 && layer < NUM_LAYERS) {
    return (grid[x][y] >> layer) & 1;
  }
  return false;
}

void initialize_grid() {
  for (int x = 0; x < WIDTH; x++) {
    for (int y = 0; y < HEIGHT; y++) {
      grid[x][y] = 0;
      for (int layer = 0; layer < NUM_LAYERS; layer++) {
        set_cell(grid, x, y, layer, random(100) < 15);
      }
    }
  }
}

int count_neighbors(int x, int y, int layer) {
  int count = 0;
  for (int i = -1; i <= 1; i++) {
    for (int j = -1; j <= 1; j++) {
      if (i == 0 && j == 0) continue;
      int nx = x + i;
      int ny = y + j;
      count += get_cell(grid, nx, ny, layer);
    }
  }
  return count;
}

void update_grid() {
  for (int x = 0; x < WIDTH; x++) {
    for (int y = 0; y < HEIGHT; y++) {
      new_grid[x][y] = 0;
      for (int layer = 0; layer < NUM_LAYERS; layer++) {
        int neighbors = count_neighbors(x, y, layer);
        if (get_cell(grid, x, y, layer) && (neighbors == 2 || neighbors == 3)) {
          set_cell(new_grid, x, y, layer, true);
        } else if (!get_cell(grid, x, y, layer) && neighbors == 3) {
          set_cell(new_grid, x, y, layer, true);
        } else {
          set_cell(new_grid, x, y, layer, false);
        }
      }
    }
  }
  for (int x = 0; x < WIDTH; x++) {
    for (int y = 0; y < HEIGHT; y++) {
      grid[x][y] = new_grid[x][y];
    }
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
  randomSeed(random);
}

void setup() {

  seed_random_from_rosc();

  tft.TFTInitSPIType(62500, spi1);
  tft.TFTSetupGPIO(PIN_RST, PIN_DC, PIN_CS, PIN_SCK, PIN_MOSI);
  tft.TFTInitScreenSize(26, 1, WIDTH, HEIGHT);
  tft.TFTInitPCBType(TFT_PCBtype_e::TFT_ST7735S_Black);
  tft.TFTchangeInvertMode(true);
  tft.TFTfillScreen(ST7735_BLACK);

  initialize_grid();

}

void loop() {

  update_grid();

  for (int y = 0; y < HEIGHT; y++) {

    for (int x = 0; x < WIDTH; x++) {

      int max_layer = -1;
      for (int layer = NUM_LAYERS - 1; layer >= 0; layer--) {
        if (get_cell(grid, x, y, layer)) {
          max_layer = layer;
          break;
        }
      }

      uint16_t color = (max_layer >= 0) ? layer_colors[max_layer] : ST7735_BLACK;
      tft.pushColor(color);

    }
  
  }

}