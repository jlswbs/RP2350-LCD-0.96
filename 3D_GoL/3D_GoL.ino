// 3D Game of Life cellular automata with 2D projection //

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

#define CELL_DRAW_SIZE 2
#define CELL_SPACING 1
#define CELL_SIZE (CELL_DRAW_SIZE + CELL_SPACING)

#define SIZE_X (WIDTH / CELL_SIZE)
#define SIZE_Y (HEIGHT / CELL_SIZE)
#define SIZE_Z (HEIGHT / CELL_SIZE)

uint8_t grid[SIZE_X][SIZE_Y][SIZE_Z];
uint8_t new_grid[SIZE_X][SIZE_Y][SIZE_Z];
uint8_t display_grid[SIZE_X][SIZE_Y];

void initialize_grid() {
  for (int x = 0; x < SIZE_X; x++) {
    for (int y = 0; y < SIZE_Y; y++) {
      for (int z = 0; z < SIZE_Z; z++) {
        grid[x][y][z] = (random(100) < 30) ? 1 : 0;
      }
    }
  }
}

int count_neighbors(int x, int y, int z) {
  int count = 0;
  for (int i = -1; i <= 1; i++) {
    for (int j = -1; j <= 1; j++) {
      for (int k = -1; k <= 1; k++) {
        if (i == 0 && j == 0 && k == 0) continue;
        int nx = (x + i + SIZE_X) % SIZE_X;
        int ny = (y + j + SIZE_Y) % SIZE_Y;
        int nz = (z + k + SIZE_Z) % SIZE_Z;
        count += grid[nx][ny][nz];
      }
    }
  }
  return count;
}

void update_grid() {
  for (int x = 0; x < SIZE_X; x++) {
    for (int y = 0; y < SIZE_Y; y++) {
      for (int z = 0; z < SIZE_Z; z++) {
        int neighbors = count_neighbors(x, y, z);
        if (grid[x][y][z] == 1 && neighbors >= 5 && neighbors <= 7) {
          new_grid[x][y][z] = 1;
        } else if (grid[x][y][z] == 0 && neighbors == 6) {
          new_grid[x][y][z] = 1;
        } else {
          new_grid[x][y][z] = 0;
        }
      }
    }
  }

  for (int x = 0; x < SIZE_X; x++) {
    for (int y = 0; y < SIZE_Y; y++) {
      for (int z = 0; z < SIZE_Z; z++) {
        grid[x][y][z] = new_grid[x][y][z];
      }
    }
  }
}

void prepare_display_grid() {
  for (int x = 0; x < SIZE_X; x++) {
    for (int y = 0; y < SIZE_Y; y++) {
      display_grid[x][y] = 0;
    }
  }

  for (int x = 0; x < SIZE_X; x++) {
    for (int y = 0; y < SIZE_Y; y++) {
      int max_z = -1;
      for (int z = 0; z < SIZE_Z; z++) {
        if (grid[x][y][z] == 1) {
          max_z = z;
        }
      }
      if (max_z >= 0) {
        display_grid[x][y] = (max_z * 255) / SIZE_Z;
      }
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

  prepare_display_grid();

  for (int x = 0; x < SIZE_X; x++) {

    for (int y = 0; y < SIZE_Y; y++) {

      uint8_t coll = display_grid[x][y];
      tft.TFTfillRectangle(CELL_SIZE * x, CELL_SIZE * y, CELL_DRAW_SIZE, CELL_DRAW_SIZE, tft.Color565(coll, coll, coll));

    }

  }

}