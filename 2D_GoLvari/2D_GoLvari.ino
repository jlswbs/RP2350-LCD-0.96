// 2D varicap base GoL cellular automata // 

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

#define V_LOCAL  2.5f
#define V_FIELD  1.5f

typedef struct {
    bool  alive;
    float energy;
    float capacitance;
} Cell;

ST7735_TFT tft;

Cell grid[SCR];
Cell new_grid[SCR];

#define IDX(x, y) ((y) * WIDTH + (x))

static inline float varicap(float voltage) {

    return 1.0f / (1.0f + 3.0f * voltage);

}

uint16_t cell_to_color(Cell *cell) {

    float hue = cell->capacitance * 360.0f;
    float saturation = 0.6f + 0.4f * cell->energy;
    float value = cell->alive ? (0.4f + 0.6f * cell->energy) : 0.15f;

    while (hue >= 360.0f) hue -= 360.0f;

    float hh = hue / 60.0f;
    int i = (int)hh;
    float ff = hh - i;

    float p = value * (1.0f - saturation);
    float q = value * (1.0f - (saturation * ff));
    float t = value * (1.0f - (saturation * (1.0f - ff)));

    float r_f, g_f, b_f;

    switch (i) {
      case 0: r_f = value; g_f = t;     b_f = p;     break;
      case 1: r_f = q;     g_f = value; b_f = p;     break;
      case 2: r_f = p;     g_f = value; b_f = t;     break;
      case 3: r_f = p;     g_f = q;     b_f = value; break;
      case 4: r_f = t;     g_f = p;     b_f = value; break;
      default:r_f = value; g_f = p;     b_f = q;     break;
    }

    int r = (int)(r_f * 31.0f);
    int g = (int)(g_f * 63.0f);
    int b = (int)(b_f * 31.0f);

    if (r < 0) r = 0; if (r > 31) r = 31;
    if (g < 0) g = 0; if (g > 63) g = 63;
    if (b < 0) b = 0; if (b > 31) b = 31;

    return (r << 11) | (g << 5) | b;

}

void update_automaton() {

    for (int y = 0; y < HEIGHT; y++) {
        int y_up   = (y - 1 + HEIGHT) % HEIGHT;
        int y_down = (y + 1) % HEIGHT;

        for (int x = 0; x < WIDTH; x++) {
            int idx = IDX(x, y);
            int x_left  = (x - 1 + WIDTH) % WIDTH;
            int x_right = (x + 1) % WIDTH;

            int live_neighbors = 0;
            float neighbor_energy = 0.0f;

            #define N(ix,iy) if (grid[IDX(ix,iy)].alive){ \
                live_neighbors++; \
                neighbor_energy += grid[IDX(ix,iy)].energy; }

            N(x_left,y_up);  N(x,y_up);  N(x_right,y_up);
            N(x_left,y);                   N(x_right,y);
            N(x_left,y_down); N(x,y_down); N(x_right,y_down);

            float avgE = (live_neighbors > 0) ?
                          neighbor_energy / live_neighbors : 0.0f;

            Cell current = grid[idx];
            Cell next;

            float voltage = V_LOCAL * current.energy + V_FIELD * avgE;
            float C = varicap(voltage);

            bool classical_next =
                current.alive ?
                (live_neighbors == 2 || live_neighbors == 3) :
                (live_neighbors == 3);

            if (current.alive) {
                if (C < 0.35f && live_neighbors >= 4)
                    classical_next = false;

                if (C > 0.7f && live_neighbors == 1)
                    classical_next = true;
            } else {
                if (C > 0.75f && live_neighbors == 2)
                    classical_next = true;
            }

            next.alive = classical_next;

            if (next.alive) {
                next.energy =
                    current.energy * 0.85f +
                    avgE * 0.2f +
                    0.05f;
            } else {
                next.energy = current.energy * 0.6f;
            }

            if (next.energy < 0.0f) next.energy = 0.0f;
            if (next.energy > 1.0f) next.energy = 1.0f;

            next.capacitance = C * 0.85f;

            new_grid[idx] = next;
        }
    }

    memcpy(grid, new_grid, sizeof(grid));

}

void rndrule() {

    for (int i = 0; i < SCR; i++) {
        grid[i].alive = (rand() % 100) < 20;
        grid[i].energy = (float)rand() / (float)RAND_MAX;
        grid[i].capacitance = varicap(grid[i].energy);
    }

}

static inline void seed_random_from_rosc() {

    uint32_t random = 0;
    volatile uint32_t *rnd_reg =
        (uint32_t *)(ROSC_BASE + ROSC_RANDOMBIT_OFFSET);

    for (int k = 0; k < 32; k++) {
        uint32_t bit;
        do {
            bit = (*rnd_reg) & 1;
        } while (bit == ((*rnd_reg) & 1));
        random = (random << 1) | bit;
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
        uint16_t color = cell_to_color(&grid[i]) << 1;
        tft.pushColor(color);
    }

}