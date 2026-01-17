// 2D memristor base GoL cellular automata // 

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

#define MEM_GAIN 0.3f

typedef struct {
    bool alive;
    float energy;
    float memory;
} Cell;

ST7735_TFT tft;

Cell grid[SCR];
Cell new_grid[SCR];

#define IDX(x, y) ((y) * WIDTH + (x))

uint16_t cell_to_color(Cell *cell) {

  float hue = cell->memory * 360.0f;
  float saturation = 0.7f + 0.3f * cell->energy;
  float value = 0.3f + 0.7f * (cell->alive ? cell->energy : 0.3f);
    
  while (hue >= 360.0f) hue -= 360.0f;
    
  if (saturation < 0.1f) {
    int val = (int)(value * 31);
    if (val > 31) val = 31;
    return (val << 11) | (val << 5) | val;
  }
    
  float hh = hue / 60.0f;
  int i = (int)hh;
  float ff = hh - i;
    
  float p = value * (1.0f - saturation);
  float q = value * (1.0f - (saturation * ff));
  float t = value * (1.0f - (saturation * (1.0f - ff)));
    
  float r_f, g_f, b_f;
    
  switch (i) {
    case 0: r_f = value; g_f = t; b_f = p; break;
    case 1: r_f = q; g_f = value; b_f = p; break;
    case 2: r_f = p; g_f = value; b_f = t; break;
    case 3: r_f = p; g_f = q; b_f = value; break;
    case 4: r_f = t; g_f = p; b_f = value; break;
    default: r_f = value; g_f = p; b_f = q; break;
  }
    
  int r = (int)(r_f * 31);
  int g = (int)(g_f * 63);
  int b = (int)(b_f * 31);
    
  if (r > 31) r = 31; else if (r < 0) r = 0;
  if (g > 63) g = 63; else if (g < 0) g = 0;
  if (b > 31) b = 31; else if (b < 0) b = 0;
    
  return (r << 11) | (g << 5) | b;

}

void update_automaton() {

  for (int y = 0; y < HEIGHT; y++) {
    int y_up = (y - 1 + HEIGHT) % HEIGHT;
    int y_down = (y + 1) % HEIGHT;
        
    for (int x = 0; x < WIDTH; x++) {
      int idx = IDX(x, y);
      int x_left = (x - 1 + WIDTH) % WIDTH;
      int x_right = (x + 1) % WIDTH;
      int live_neighbors = 0;
      float neighbor_energy = 0;
            
      if (grid[IDX(x_left, y_up)].alive) { live_neighbors++; neighbor_energy += grid[IDX(x_left, y_up)].energy; }
      if (grid[IDX(x, y_up)].alive) { live_neighbors++; neighbor_energy += grid[IDX(x, y_up)].energy; }
      if (grid[IDX(x_right, y_up)].alive) { live_neighbors++; neighbor_energy += grid[IDX(x_right, y_up)].energy; }
            
      if (grid[IDX(x_left, y)].alive) { live_neighbors++; neighbor_energy += grid[IDX(x_left, y)].energy; }
      if (grid[IDX(x_right, y)].alive) { live_neighbors++; neighbor_energy += grid[IDX(x_right, y)].energy; }
            
      if (grid[IDX(x_left, y_down)].alive) { live_neighbors++; neighbor_energy += grid[IDX(x_left, y_down)].energy; }
      if (grid[IDX(x, y_down)].alive) { live_neighbors++; neighbor_energy += grid[IDX(x, y_down)].energy; }
      if (grid[IDX(x_right, y_down)].alive) { live_neighbors++; neighbor_energy += grid[IDX(x_right, y_down)].energy; }
            
      float avg_neighbor_energy = (live_neighbors > 0) ? neighbor_energy / live_neighbors : 0;
            
      bool classical_alive = grid[idx].alive;
      bool classical_next = classical_alive ? (live_neighbors == 2 || live_neighbors == 3) : (live_neighbors == 3);
            
      Cell current = grid[idx];
      Cell next;

      next.alive = classical_next;

      if (classical_alive && current.energy > 0.7f) {
        if (live_neighbors >= 1 && live_neighbors <= 4) {
          next.alive = true;
        }
      }
            
      if (classical_alive && current.energy < 0.3f) {
        if (live_neighbors < 2 || live_neighbors > 3) {
          next.alive = false;
        }
      }
            
      if (!classical_alive && avg_neighbor_energy > 0.6f) {
        if (live_neighbors >= 2 && live_neighbors <= 3) {
          next.alive = true;
        }
      }
            
      if (current.memory > 0.8f && classical_alive) {
        if (live_neighbors < 1 || live_neighbors > 4) {
          next.alive = false;
        }
      }

      if (next.alive) {
        next.energy = current.energy * 0.85f + 0.15f;
        if (avg_neighbor_energy > 0.5f) {
          next.energy += 0.25f;
        }
        next.memory = fminf(1.0f, current.memory + MEM_GAIN);
      } else {
                next.energy = current.energy * 0.7f;
                next.memory = current.memory * 0.95f;
        if (avg_neighbor_energy > 0.3f) {
          next.energy += 0.02f;
        }
      }

      if (next.energy > 1.0f) next.energy = 1.0f;
      if (next.energy < 0.0f) next.energy = 0.0f;
      if (next.memory > 1.0f) next.memory = 1.0f;
      if (next.memory < 0.0f) next.memory = 0.0f;
            
      if (fabsf(current.energy - avg_neighbor_energy) < 0.2f) {
        next.energy = (current.energy + avg_neighbor_energy) * 0.5f;
      }
            
      new_grid[idx] = next;
    }
  }
    
  memcpy(grid, new_grid, sizeof(grid));
}

void rndrule() {

  for (int i = 0; i < SCR; i++) {

    grid[i].alive = (rand() % 100) < 20;
    grid[i].energy = (float)rand() / (float)RAND_MAX;
    grid[i].memory = (float)rand() / (float)RAND_MAX * 0.5f;

  }

}

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
    
  for (int i = 0; i < SCR; i++) {
    uint16_t color = cell_to_color(&grid[i])<<1;
    tft.pushColor(color);
  }

}