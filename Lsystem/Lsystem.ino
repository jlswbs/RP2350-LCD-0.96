// L-system fractal //

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

const char* axiom = "X";
const char* rules[] = {"X=F[+X][-X]FX", "F=FF"};
const int iterations = 5;
const float length = 2.3f;
const float base_angle = 20.0f;

char lsystem[10000];
float x, y, angle;
struct Stack { float x, y, angle; };
Stack stack[2000];
int stack_ptr = 0;

void generate_lsystem() {

  strcpy(lsystem, axiom);
  char temp[10000];
  for (int i = 0; i < iterations; i++) {
    strcpy(temp, "");
    for (int j = 0; j < strlen(lsystem); j++) {
      char c = lsystem[j];
      if (c == 'X') strcat(temp, rules[0] + 2);
      else if (c == 'F') strcat(temp, rules[1] + 2);
      else {
        char tmp[2] = {c, '\0'};
        strcat(temp, tmp);
      }
    }
    strcpy(lsystem, temp);
  }

}

void draw_lsystem(float dynamic_angle) {

  stack_ptr = 0;
  x = WIDTH / 2;
  y = HEIGHT - 10;
  angle = -90;

  tft.TFTfillScreen(ST7735_BLACK);

  for (int i = 0; i < strlen(lsystem); i++) {
    char c = lsystem[i];
    if (c == 'F') {
      float rad = angle * PI / 180.0;
      float new_x = x + length * cosf(rad);
      float new_y = y + length * sinf(rad);
      tft.TFTdrawLine((int)x, (int)y, (int)new_x, (int)new_y, ST7735_WHITE);
      x = new_x;
      y = new_y;
    } else if (c == '+') {
      angle += dynamic_angle;
    } else if (c == '-') {
      angle -= dynamic_angle;
    } else if (c == '[') {
      if (stack_ptr >= 2000) return; // Kontrola přetečení zásobníku
      stack[stack_ptr++] = {x, y, angle};
    } else if (c == ']') {
      if (stack_ptr <= 0) return; // Kontrola podtečení zásobníku
      Stack s = stack[--stack_ptr];
      x = s.x;
      y = s.y;
      angle = s.angle;
    }
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

void setup(){

  seed_random_from_rosc();

  tft.TFTInitSPIType(62500, spi1);
	tft.TFTSetupGPIO(PIN_RST, PIN_DC, PIN_CS, PIN_SCK, PIN_MOSI);
  tft.TFTInitScreenSize(26, 1, WIDTH, HEIGHT);
	tft.TFTInitPCBType(TFT_PCBtype_e::TFT_ST7735S_Black);
  tft.TFTchangeInvertMode(true);
	tft.TFTfillScreen(ST7735_BLACK);
  
  generate_lsystem();
  
}

void loop(){

  unsigned long t = millis();
  float dynamic_angle = base_angle + 5.0f * sinf(t * 0.002f);

  draw_lsystem(dynamic_angle);
  delay(15);

}