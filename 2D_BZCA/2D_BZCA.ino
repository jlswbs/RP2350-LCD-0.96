// 2D Belousov-Zhabotinsky cellular automata //

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

#define NUMS    5

  uint8_t cells[WIDTH][HEIGHT];
  uint8_t nextcells[WIDTH][HEIGHT];
  int dir[2][4] = {{0, 2, 0, -2},{-2, 0, 2, 0}};

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
  
}

void loop(){
  
  for(int i = 0; i < NUMS; i++) cells[rand()%WIDTH][rand()%HEIGHT] = rand()%3;

  for(int y = 0; y < HEIGHT; y=y+2){
    
    for(int x = 0; x < WIDTH; x=x+2){
      
      if(cells[x][y] == 1) nextcells[x][y] = 2;
      else if(cells[x][y] == 2) nextcells[x][y] = 0;
      else {
        
        nextcells[x][y] = 0;
        
        for(int i = 0; i < 4; i++){
          int dx = x + dir[0][i];
          int dy = y + dir[1][i];
          if(0 <= dx && dx < WIDTH && 0 <= dy && dy < HEIGHT && cells[dx][dy] == 1) nextcells[x][y] = 1;
        }
      }
    }
  }
  
  for(int y = 0; y < HEIGHT; y++){
    
    for(int x = 0; x < WIDTH; x++) cells[x][y] = nextcells[x][y];
    
  }

  for(int y = 0; y < HEIGHT; y++){
    
    for(int x = 0; x < WIDTH; x++){

      uint16_t image;
      if(cells[x][y] == 0) image = ST7735_BLACK;
      else if(cells[x][y] == 1) image = ST7735_BLUE;
      else image = ST7735_WHITE;
      tft.pushColor(image);
      
    }
    
  }

}