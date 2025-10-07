// 2D Wolfram cellular automata //

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

  bool state[SCR];
  bool newstate[SCR];
  bool rules[10];
  uint16_t image;

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

void rndrule(){

  for(int i = 0; i < 10; i++) rules[i] = rand()%2;

  memset(newstate, 0, SCR);
  memset(state, 0, SCR);
  
  state[(WIDTH/2)+(HEIGHT/2)*WIDTH] = 1;
  state[(WIDTH/2)+((HEIGHT/2)-1)*WIDTH] = 1;
  state[((WIDTH/2)-1)+((HEIGHT/2)-1)*WIDTH] = 1;
  state[((WIDTH/2)-1)+(HEIGHT/2)*WIDTH] = 1;

}

uint8_t neighbors(uint16_t x, uint16_t y) {
  
  uint8_t result = 0;

  if(y > 0 && state[x+(y-1)*WIDTH] == 1) result = result + 1;
  if(x > 0 && state[(x-1)+y*WIDTH] == 1) result = result + 1;
  if(x < WIDTH-1 && state[(x+1)+y*WIDTH] == 1) result = result + 1;
  if(y < HEIGHT-1 && state[x+(y+1)*WIDTH] == 1) result = result + 1;
  
  return result;
 
}


void setup(){

  seed_random_from_rosc();

  tft.TFTInitSPIType(62500, spi1);
	tft.TFTSetupGPIO(PIN_RST, PIN_DC, PIN_CS, PIN_SCK, PIN_MOSI);
  tft.TFTInitScreenSize(26, 1, WIDTH, HEIGHT);
	tft.TFTInitPCBType(TFT_PCBtype_e::TFT_ST7735S_Black);
  tft.TFTchangeInvertMode(true);
	tft.TFTfillScreen(ST7735_BLACK);

  rndrule();
  
}

void loop(){

  for(int y = 0; y < HEIGHT; y++){
    
    for(int x = 0; x < WIDTH; x++){
    
      uint8_t totalNeighbors = neighbors(x, y);
          
      if(state[x+y*WIDTH] == 0 && totalNeighbors == 0)      {newstate[x+y*WIDTH] = rules[0]; image = ST7735_WHITE;}
      else if(state[x+y*WIDTH] == 1 && totalNeighbors == 0) {newstate[x+y*WIDTH] = rules[1]; image = ST7735_RED;}
      else if(state[x+y*WIDTH] == 0 && totalNeighbors == 1) {newstate[x+y*WIDTH] = rules[2]; image = ST7735_GREEN;}
      else if(state[x+y*WIDTH] == 1 && totalNeighbors == 1) {newstate[x+y*WIDTH] = rules[3]; image = ST7735_BLUE;}
      else if(state[x+y*WIDTH] == 0 && totalNeighbors == 2) {newstate[x+y*WIDTH] = rules[4]; image = ST7735_YELLOW;}
      else if(state[x+y*WIDTH] == 1 && totalNeighbors == 2) {newstate[x+y*WIDTH] = rules[5]; image = ST7735_BLUE;}
      else if(state[x+y*WIDTH] == 0 && totalNeighbors == 3) {newstate[x+y*WIDTH] = rules[6]; image = ST7735_MAGENTA;}
      else if(state[x+y*WIDTH] == 1 && totalNeighbors == 3) {newstate[x+y*WIDTH] = rules[7]; image = ST7735_CYAN;}
      else if(state[x+y*WIDTH] == 0 && totalNeighbors == 4) {newstate[x+y*WIDTH] = rules[8]; image = ST7735_RED;}
      else if(state[x+y*WIDTH] == 1 && totalNeighbors == 4) {newstate[x+y*WIDTH] = rules[9]; image = ST7735_BLACK;}

      tft.pushColor(image);
      
    }
  }
 
  memcpy(state, newstate, SCR);

}