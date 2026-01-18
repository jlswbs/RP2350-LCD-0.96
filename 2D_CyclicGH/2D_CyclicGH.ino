// 2D Cyclic cellular automata Greenberg-Hastings rule //

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

uint8_t grid[SCR];
uint8_t new_grid[SCR];

#define N_STATES  3
#define MUTATE    0.0004f

#define IDX(x, y) ((y) * WIDTH + (x))

uint16_t cell_to_color(uint8_t state) {

  switch (state) {
    case 0:
      return 0x0000;
    case 1:
      return 0xF800;  
    case 2:
      return 0x001F;     
    default:
      return 0x0000;
  }

}

void rndrule() {

  for (int i = 0; i < 8; i++) {

    int x = rand() % WIDTH;
    int y = rand() % HEIGHT;
    
    for (int dy = -1; dy <= 1; dy++) {
      for (int dx = -1; dx <= 1; dx++) {
        int nx = (x + dx + WIDTH) % WIDTH;
        int ny = (y + dy + HEIGHT) % HEIGHT;
        grid[IDX(nx, ny)] = 1;
      }
    }
  }

}

void update_automaton() {

  for (int y = 0; y < HEIGHT; y++) {

    for (int x = 0; x < WIDTH; x++) {

      int idx = IDX(x, y);
      uint8_t current = grid[idx];
      uint8_t next_state = current;
      
      if (current == 1) {
        next_state = 2;
      } else if (current == 2) {
        next_state = 0;
      } else if (current == 0) {
        for (int dy = -1; dy <= 1; dy++) {
          for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue;
            
            int nx = (x + dx + WIDTH) % WIDTH;
            int ny = (y + dy + HEIGHT) % HEIGHT;
            
            if (grid[IDX(nx, ny)] == 1) {
              next_state = 1;
              goto next_cell;
            }
          }
        }
      }
      
      next_cell:
      float rand_val = (float)rand() / (float)RAND_MAX;
      if (rand_val < MUTATE) next_state = (next_state + 1) % N_STATES;
      new_grid[idx] = next_state;

    }
  }

  memcpy(grid, new_grid, sizeof(grid));

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
    
  rndrule();

}

void loop() {

  update_automaton();
    
  for (int i = 0; i < SCR; i++) tft.pushColor(cell_to_color(grid[i]));

}