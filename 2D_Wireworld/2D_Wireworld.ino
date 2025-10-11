// 2D Wireworld cellular automata //

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

#define NUM_GATES 3
#define NUM_SWITCHES 3
#define NUM_OSCILLATORS 2
#define NUM_DIODES 2
#define NUM_ABSORBERS 6
#define NUM_ELECTRONS 1
#define PERCENT_CONNECTIONS 10
#define MAX_COMPONENTS (NUM_GATES + NUM_SWITCHES + NUM_OSCILLATORS + NUM_DIODES + NUM_ABSORBERS)
#define MAX_CONNECTIONS (2 * NUM_GATES + 3 * NUM_SWITCHES)

  uint16_t color;
  uint8_t grid[WIDTH][HEIGHT];
  uint8_t new_grid[WIDTH][HEIGHT];
  int positions[MAX_COMPONENTS][2];
  int connection_points[MAX_CONNECTIONS][3];
  int pos_count = 0;
  int conn_count = 0;


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

void place_or_gate(int x, int y) {
  if (x >= 3 && x < WIDTH - 3 && y >= 3 && y < HEIGHT - 3) {
    grid[x][y] = 3;
    grid[x-1][y-1] = 3;
    grid[x+1][y-1] = 3;
    grid[x][y+1] = 3;
    grid[x][y+2] = 3;
  }
}

void place_and_gate(int x, int y) {
  if (x >= 3 && x < WIDTH - 3 && y >= 3 && y < HEIGHT - 3) {
    grid[x][y] = 3;
    grid[x-1][y-1] = 3;
    grid[x+1][y-1] = 3;
    grid[x][y+2] = 3;
    grid[x][y+3] = 3;
  }
}

void place_xor_gate(int x, int y) {
  if (x >= 5 && x < WIDTH - 5 && y >= 5 && y < HEIGHT - 5) {
    grid[x][y] = 3;
    grid[x-1][y-1] = 3;
    grid[x+1][y-1] = 3;
    grid[x-2][y] = 3;
    grid[x+2][y] = 3;
    grid[x-3][y+1] = 3;
    grid[x+3][y+1] = 3;
    grid[x-2][y+2] = 3;
    grid[x+2][y+2] = 3;
    grid[x-1][y+3] = 3;
    grid[x+1][y+3] = 3;
    grid[x][y+4] = 3;
    grid[x][y+5] = 3;
  }
}

void place_oscillator(int x, int y) {
  if (x >= 2 && x < WIDTH - 2 && y >= 2 && y < HEIGHT - 2) {
    grid[x][y] = 3;
    grid[x+1][y] = 3;
    grid[x][y+1] = 3;
    grid[x][y] = 1;
  }
}

void place_switch(int x, int y) {
  if (x >= 3 && x < WIDTH - 3 && y >= 3 && y < HEIGHT - 3) {
    grid[x][y] = 3;
    grid[x-1][y] = 3;
    grid[x+1][y] = 3;
    grid[x][y+1] = 3;
  }
}

void place_diode(int x, int y, int dir) {
  if (x >= 2 && x < WIDTH - 2 && y >= 2 && y < HEIGHT - 2) {
    grid[x][y] = 3;
    int dx[4] = {1, -1, 0, 0};
    int dy[4] = {0, 0, 1, -1};
    for (int i = 1; i <= 2; i++) {
      int nx = x + dx[dir] * i;
      int ny = y + dy[dir] * i;
      if (nx >= 0 && nx < WIDTH && ny >= 0 && ny < HEIGHT) {
        grid[nx][ny] = 3;
      }
    }
  }
}

void place_absorber(int x, int y, int dir) {
  if (x >= 2 && x < WIDTH - 2 && y >= 2 && y < HEIGHT - 2) {
    grid[x][y] = 3;
    int dx[4] = {1, -1, 0, 0};
    int dy[4] = {0, 0, 1, -1};
    for (int i = 1; i <= 2; i++) {
      int nx = x + dx[dir] * i;
      int ny = y + dy[dir] * i;
      if (nx >= 0 && nx < WIDTH && ny >= 0 && ny < HEIGHT) {
        grid[nx][ny] = 3;
      }
    }
  }
}

void connect_elements(int x1, int y1, int x2, int y2) {
  if (x1 == x2) {
    int start_y = min(y1, y2);
    int end_y = max(y1, y2);
    for (int y = start_y; y <= end_y; y++) {
      if (y >= 0 && y < HEIGHT) {
        grid[x1][y] = 3;
      }
    }
  } else if (y1 == y2) {
    int start_x = min(x1, x2);
    int end_x = max(x1, x2);
    for (int x = start_x; x <= end_x; x++) {
      if (x >= 0 && x < WIDTH) {
        grid[x][y1] = 3;
      }
    }
  } else {
    int mid_x = (x1 + x2) / 2;
    for (int x = min(x1, mid_x); x <= max(x1, mid_x); x++) {
      if (x >= 0 && x < WIDTH) {
        grid[x][y1] = 3;
      }
    }
    for (int y = min(y1, y2); y <= max(y1, y2); y++) {
      if (y >= 0 && y < HEIGHT) {
        grid[mid_x][y] = 3;
      }
    }
    for (int x = min(mid_x, x2); x <= max(mid_x, x2); x++) {
      if (x >= 0 && x < WIDTH) {
        grid[x][y2] = 3;
      }
    }
  }
}

void rndseed() {
  for (int x = 0; x < WIDTH; x++) {
    for (int y = 0; y < HEIGHT; y++) {
      grid[x][y] = 0;
    }
  }
  pos_count = 0;
  conn_count = 0;

  for (int i = 0; i < NUM_GATES; i++) {
    int x, y;
    do {
      x = 5 + random(WIDTH - 11);
      y = 5 + random(HEIGHT - 11);
    } while (grid[x][y] != 0);
    int gate_type = random(3);
    if (gate_type == 0) {
      place_or_gate(x, y);
      if (conn_count < MAX_CONNECTIONS) {
        connection_points[conn_count][0] = x-1; connection_points[conn_count][1] = y-1; connection_points[conn_count][2] = 0; // input
        conn_count++;
      }
      if (conn_count < MAX_CONNECTIONS) {
        connection_points[conn_count][0] = x+1; connection_points[conn_count][1] = y-1; connection_points[conn_count][2] = 0; // input
        conn_count++;
      }
      if (conn_count < MAX_CONNECTIONS) {
        connection_points[conn_count][0] = x; connection_points[conn_count][1] = y+2; connection_points[conn_count][2] = 1; // output
        conn_count++;
      }
    } else if (gate_type == 1) {
      place_and_gate(x, y);
      if (conn_count < MAX_CONNECTIONS) {
        connection_points[conn_count][0] = x-1; connection_points[conn_count][1] = y-1; connection_points[conn_count][2] = 0; // input
        conn_count++;
      }
      if (conn_count < MAX_CONNECTIONS) {
        connection_points[conn_count][0] = x+1; connection_points[conn_count][1] = y-1; connection_points[conn_count][2] = 0; // input
        conn_count++;
      }
      if (conn_count < MAX_CONNECTIONS) {
        connection_points[conn_count][0] = x; connection_points[conn_count][1] = y+3; connection_points[conn_count][2] = 1; // output
        conn_count++;
      }
    } else {
      place_xor_gate(x, y);
      if (conn_count < MAX_CONNECTIONS) {
        connection_points[conn_count][0] = x-1; connection_points[conn_count][1] = y-1; connection_points[conn_count][2] = 0; // input
        conn_count++;
      }
      if (conn_count < MAX_CONNECTIONS) {
        connection_points[conn_count][0] = x+1; connection_points[conn_count][1] = y-1; connection_points[conn_count][2] = 0; // input
        conn_count++;
      }
      if (conn_count < MAX_CONNECTIONS) {
        connection_points[conn_count][0] = x; connection_points[conn_count][1] = y+5; connection_points[conn_count][2] = 1; // output
        conn_count++;
      }
    }
    if (pos_count < MAX_COMPONENTS) {
      positions[pos_count][0] = x;
      positions[pos_count][1] = y;
      pos_count++;
    }
  }

  for (int i = 0; i < NUM_SWITCHES; i++) {
    int x, y;
    do {
      x = 3 + random(WIDTH - 7);
      y = 3 + random(HEIGHT - 7);
    } while (grid[x][y] != 0);
    place_switch(x, y);
    if (conn_count < MAX_CONNECTIONS) {
      connection_points[conn_count][0] = x; connection_points[conn_count][1] = y+1; connection_points[conn_count][2] = 0; // input
      conn_count++;
    }
    if (conn_count < MAX_CONNECTIONS) {
      connection_points[conn_count][0] = x-1; connection_points[conn_count][1] = y; connection_points[conn_count][2] = 1; // output
      conn_count++;
    }
    if (conn_count < MAX_CONNECTIONS) {
      connection_points[conn_count][0] = x+1; connection_points[conn_count][1] = y; connection_points[conn_count][2] = 1; // output
      conn_count++;
    }
    if (pos_count < MAX_COMPONENTS) {
      positions[pos_count][0] = x;
      positions[pos_count][1] = y;
      pos_count++;
    }
  }

  for (int i = 0; i < NUM_OSCILLATORS; i++) {
    int x, y;
    do {
      x = 3 + random(WIDTH - 7);
      y = 3 + random(HEIGHT - 7);
    } while (grid[x][y] != 0);
    place_oscillator(x, y);
    if (pos_count < MAX_COMPONENTS) {
      positions[pos_count][0] = x;
      positions[pos_count][1] = y;
      pos_count++;
    }
  }

  for (int i = 0; i < NUM_DIODES; i++) {
    int x, y;
    do {
      x = 3 + random(WIDTH - 7);
      y = 3 + random(HEIGHT - 7);
    } while (grid[x][y] != 0);
    int dir = random(4);
    place_diode(x, y, dir);
    if (pos_count < MAX_COMPONENTS) {
      positions[pos_count][0] = x;
      positions[pos_count][1] = y;
      pos_count++;
    }
  }

  for (int i = 0; i < NUM_ABSORBERS; i++) {
    int x, y;
    do {
      x = 3 + random(WIDTH - 7);
      y = 3 + random(HEIGHT - 7);
    } while (grid[x][y] != 0);
    int dir = random(4);
    place_absorber(x, y, dir);
    if (pos_count < MAX_COMPONENTS) {
      positions[pos_count][0] = x;
      positions[pos_count][1] = y;
      pos_count++;
    }
  }

  for (int i = 0; i < conn_count; i++) {
    int x = connection_points[i][0];
    int y = connection_points[i][1];
    int io_type = connection_points[i][2];
    int attempts = 0;
    bool connected = false;
    while (attempts < 10 && !connected && pos_count > 1) {
      int j = random(pos_count);
      int x2 = positions[j][0];
      int y2 = positions[j][1];
      if ((x2 != x || y2 != y) && grid[x2][y2] == 3) {
        connect_elements(x, y, x2, y2);
        connected = true;
      }
      attempts++;
    }
  }

  for (int i = 0; i < pos_count; i++) {
    for (int j = i + 1; j < pos_count; j++) {
      if (random(100) < PERCENT_CONNECTIONS) {
        connect_elements(positions[i][0], positions[i][1], positions[j][0], positions[j][1]);
      }
    }
  }

  for (int i = 0; i < NUM_ELECTRONS; i++) {
    int x, y;
    do {
      x = random(WIDTH);
      y = random(HEIGHT);
    } while (grid[x][y] != 3);
    grid[x][y] = 1;
  }
}

uint8_t count_neighbors(uint8_t x, uint8_t y) {
  uint8_t count = 0;
  for (int8_t i = -1; i <= 1; i++) {
    for (int8_t j = -1; j <= 1; j++) {
      if (i == 0 && j == 0) continue;
      int nx = x + i;
      int ny = y + j;
      if (nx >= 0 && nx < WIDTH && ny >= 0 && ny < HEIGHT && grid[nx][ny] == 1) {
        count++;
      }
    }
  }
  return count;
}

void update_grid() {
  for (uint8_t y = 0; y < HEIGHT; y++) {
    for (uint8_t x = 0; x < WIDTH; x++) {
      uint8_t current_state = grid[x][y];
      new_grid[x][y] = current_state;
      if (current_state == 0) {
        new_grid[x][y] = 0;
      } else if (current_state == 1) {
        new_grid[x][y] = 2;
      } else if (current_state == 2) {
        new_grid[x][y] = 3;
      } else if (current_state == 3) {
        uint8_t neighbors = count_neighbors(x, y);
        new_grid[x][y] = (neighbors == 1 || neighbors == 2) ? 1 : 3;
      }
    }
  }

  for (uint8_t y = 0; y < HEIGHT; y++) {
    for (uint8_t x = 0; x < WIDTH; x++) {
      grid[x][y] = new_grid[x][y];
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

      if (grid[x][y] == 0) color = ST7735_BLACK;
      else if (grid[x][y] == 1) color = ST7735_WHITE;
      else if (grid[x][y] == 2) color = ST7735_RED;
      else color = ST7735_BLUE;
      tft.pushColor(color);

    }

  }

  delay(15);

}