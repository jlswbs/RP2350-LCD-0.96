// 2D Physarum growth //

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

#define ITER  2000
#define NUM   8
#define MAX_TRAIL 100

  uint16_t grid[WIDTH][HEIGHT];
  uint16_t trail[WIDTH][HEIGHT];
  uint16_t coll[NUM];
  uint16_t image;
  int t, q;


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

  for (int i = 0; i < NUM; i++) coll[i] = rand();

  for (int y = 0; y < HEIGHT; y++){  
    for (int x = 0; x < WIDTH; x++){
      
      if(x == 0 || x == 1 || x == WIDTH-2 || x == WIDTH-1 || y == 0 || y == 1 || y == HEIGHT-2 || y == HEIGHT-1) grid[x][y] = 1;
      else grid[x][y] = 0;

    }
  }
  
  for (int i = 1; i < NUM; i++){
    
    int x = 2 * (5 + rand()%(WIDTH/2)-5);
    int y = 2 * (5 + rand()%(HEIGHT/2)-5);
    if(grid[x][y] == 0) grid[x][y] = 1000+(i*100);

  }
  
}

void nextstep(){

  for (int i = 0; i < ITER; i++){
    int x = 2 * (1 + rand()%(WIDTH/2)-1);
    int y = 2 * (1 + rand()%(HEIGHT/2)-1);
    
    if(grid[x][y] >= 1000 && grid[x][y] < 2000){
      q = (grid[x][y]/100) - 10;
      
      if(grid[x+2][y] == 0){ 
        grid[x+2][y] = q*100; 
        grid[x+1][y] = q*100;
        trail[x+1][y] = 10;
      }
      if(grid[x][y+2] == 0){ 
        grid[x][y+2] = q*100; 
        grid[x][y+1] = q*100;
        trail[x][y+1] = 10;
      }
      if(grid[x-2][y] == 0){ 
        grid[x-2][y] = q*100; 
        grid[x-1][y] = q*100;
        trail[x-1][y] = 10;
      }
      if(grid[x][y-2] == 0){ 
        grid[x][y-2] = q*100; 
        grid[x][y-1] = q*100;
        trail[x][y-1] = 10;
      }
      
      grid[x][y] = q*100;
    }
    
    if(grid[x][y] >= 100 && grid[x][y] < 1000){
      q = grid[x][y]/100;

      if(x < 2 || x >= WIDTH-2 || y < 2 || y >= HEIGHT-2) continue;
      
      int sensors[4];
      sensors[0] = trail[x+2][y];
      sensors[1] = trail[x][y+2];
      sensors[2] = trail[x-2][y];
      sensors[3] = trail[x][y-2];
      
      int maxDir = 0;
      int maxVal = sensors[0];
      for(int d = 1; d < 4; d++){
        if(sensors[d] > maxVal){
          maxVal = sensors[d];
          maxDir = d;
        }
      }

      int bias = rand() % 100;
      if(bias < 70 && maxVal > 0){
        t = maxDir + 1;
      } else {
        t = 1 + rand()%4;
      }
      
      int dx = (t==1)?2:(t==3)?-2:0;
      int dy = (t==2)?2:(t==4)?-2:0;
      
      if(x+dx < 0 || x+dx >= WIDTH || y+dy >= HEIGHT || y+dy < 0) continue;
      
      if(grid[x+dx][y+dy] == 0){
        grid[x+dx][y+dy] = q*100;
        grid[x+dx/2][y+dy/2] = q*100;
        trail[x][y] = (trail[x][y] + 5 > MAX_TRAIL) ? MAX_TRAIL : trail[x][y] + 5;
      }

      else if(grid[x+dx][y+dy] >= 100 && grid[x+dx][y+dy] < 1000){
        int otherQ = grid[x+dx][y+dy]/100;
        if(otherQ != q){
          if(rand()%2 == 0){
            grid[x+dx][y+dy] = q*100;
            grid[x+dx/2][y+dy/2] = q*100;
          }
        }
      }
    }
    
    if(trail[x][y] > 0) trail[x][y] = trail[x][y] - 1;
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

  nextstep();

  for (int y = 0; y < HEIGHT; y++){

    for (int x = 0; x < WIDTH; x++){
    
      if(grid[x][y] >= 100 && grid[x][y] < 1000){
        q = (grid[x][y] / 100) % NUM;
        image = coll[q];    
      } else image = ST7735_BLACK;

      tft.pushColor(image);
      
    }

  }

}