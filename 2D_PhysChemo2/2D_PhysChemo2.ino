// 2D Physarum chemotax trails - toroidal wraping //

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

#define NUM_SPECIES  8
#define AGENTS_PER_SPECIES 13
#define TOTAL_AGENTS (NUM_SPECIES * AGENTS_PER_SPECIES)

#define MOVE_SPEED 1.0f
#define SENSOR_DISTANCE 5.0f
#define SENSOR_ANGLE 0.8f
#define DEPOSIT_AMOUNT 100.0f
#define DECAY_RATE 0.99f
#define DIFFUSE_RATE 0.1f

struct Agent {
  float x;
  float y;
  float angle;
  int species;
};

float trail[NUM_SPECIES][WIDTH][HEIGHT];
float trailTemp[WIDTH][HEIGHT];
uint16_t coll[NUM_SPECIES];
Agent agents[TOTAL_AGENTS];

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

float randf() {
  return (float)rand() / (float)RAND_MAX;
}

uint16_t blendColors(uint16_t c1, uint16_t c2, float ratio) {
  uint8_t r1 = (c1 >> 11) & 0x1F;
  uint8_t g1 = (c1 >> 5) & 0x3F;
  uint8_t b1 = c1 & 0x1F;
  
  uint8_t r2 = (c2 >> 11) & 0x1F;
  uint8_t g2 = (c2 >> 5) & 0x3F;
  uint8_t b2 = c2 & 0x1F;
  
  uint8_t r = r1 + (uint8_t)((r2 - r1) * ratio);
  uint8_t g = g1 + (uint8_t)((g2 - g1) * ratio);
  uint8_t b = b1 + (uint8_t)((b2 - b1) * ratio);
  
  return (r << 11) | (g << 5) | b;
}

int wrapX(int x) {
  if (x < 0) return WIDTH + x;
  if (x >= WIDTH) return x - WIDTH;
  return x;
}

int wrapY(int y) {
  if (y < 0) return HEIGHT + y;
  if (y >= HEIGHT) return y - HEIGHT;
  return y;
}

float senseTrail(int species, float x, float y) {
  int ix = wrapX((int)x);
  int iy = wrapY((int)y);
  return trail[species][ix][iy];
}

float senseEnemyTrail(int species, float x, float y) {
  int ix = wrapX((int)x);
  int iy = wrapY((int)y);
  
  float enemySum = 0.0f;
  for (int s = 0; s < NUM_SPECIES; s++) {
    if (s != species) {
      enemySum += trail[s][ix][iy];
    }
  }
  return enemySum;
}

void diffuseTrail(int species) {

  for (int y = 0; y < HEIGHT; y++) {
    for (int x = 0; x < WIDTH; x++) {
      float sum = trail[species][x][y];
      
      int left = wrapX(x - 1);
      int right = wrapX(x + 1);
      int up = wrapY(y - 1);
      int down = wrapY(y + 1);
      
      sum += trail[species][left][y] + trail[species][right][y];
      sum += trail[species][x][up] + trail[species][x][down];
      sum += trail[species][left][up] + trail[species][right][up];
      sum += trail[species][left][down] + trail[species][right][down];
      
      trailTemp[x][y] = sum / 9.0f;
    }
  }
  
  for (int y = 0; y < HEIGHT; y++) {
    for (int x = 0; x < WIDTH; x++) {
      trail[species][x][y] = trail[species][x][y] * (1.0f - DIFFUSE_RATE) + 
                              trailTemp[x][y] * DIFFUSE_RATE;
    }
  }
}

void rndseed(){

  for (int i = 0; i < NUM_SPECIES; i++) {
    float hue = (float)i / (float)NUM_SPECIES;
    uint8_t r, g, b;
    if (hue < 0.166f) {
      r = 31; g = (uint8_t)(hue * 6.0f * 63); b = 0;
    } else if (hue < 0.333f) {
      r = (uint8_t)((0.333f - hue) * 6.0f * 31); g = 63; b = 0;
    } else if (hue < 0.5f) {
      r = 0; g = 63; b = (uint8_t)((hue - 0.333f) * 6.0f * 31);
    } else if (hue < 0.666f) {
      r = 0; g = (uint8_t)((0.666f - hue) * 6.0f * 63); b = 31;
    } else if (hue < 0.833f) {
      r = (uint8_t)((hue - 0.666f) * 6.0f * 31); g = 0; b = 31;
    } else {
      r = 31; g = 0; b = (uint8_t)((1.0f - hue) * 6.0f * 31);
    }
    
    coll[i] = (r << 11) | (g << 5) | b;
  }

  for (int s = 0; s < NUM_SPECIES; s++) {
    for (int y = 0; y < HEIGHT; y++){  
      for (int x = 0; x < WIDTH; x++){
        trail[s][x][y] = 0.0f;
      }
    }
  }
  
  for (int s = 0; s < NUM_SPECIES; s++){
    for (int a = 0; a < AGENTS_PER_SPECIES; a++){
        int idx = s * AGENTS_PER_SPECIES + a;
        agents[idx].x = randf() * WIDTH;
        agents[idx].y = randf() * HEIGHT;
        agents[idx].angle = randf() * 2.0f * M_PI;
        agents[idx].species = s;
    }
  }

}

void nextstep(){

  for (int i = 0; i < TOTAL_AGENTS; i++){
    Agent &ag = agents[i];

    float sensorAngleLeft = ag.angle + SENSOR_ANGLE;
    float sensorAngleRight = ag.angle - SENSOR_ANGLE;
    
    float senseForwardX = ag.x + cos(ag.angle) * SENSOR_DISTANCE;
    float senseForwardY = ag.y + sin(ag.angle) * SENSOR_DISTANCE;
    
    float senseLeftX = ag.x + cos(sensorAngleLeft) * SENSOR_DISTANCE;
    float senseLeftY = ag.y + sin(sensorAngleLeft) * SENSOR_DISTANCE;
    
    float senseRightX = ag.x + cos(sensorAngleRight) * SENSOR_DISTANCE;
    float senseRightY = ag.y + sin(sensorAngleRight) * SENSOR_DISTANCE;
    
    float weightForward = senseTrail(ag.species, senseForwardX, senseForwardY);
    float weightLeft = senseTrail(ag.species, senseLeftX, senseLeftY);
    float weightRight = senseTrail(ag.species, senseRightX, senseRightY);
    
    float enemyForward = senseEnemyTrail(ag.species, senseForwardX, senseForwardY);
    float enemyLeft = senseEnemyTrail(ag.species, senseLeftX, senseLeftY);
    float enemyRight = senseEnemyTrail(ag.species, senseRightX, senseRightY);

    weightForward -= enemyForward * 0.5f;
    weightLeft -= enemyLeft * 0.5f;
    weightRight -= enemyRight * 0.5f;
    
    float randomSteer = (randf() - 0.5f) * 0.3f;
    
    if (weightForward > weightLeft && weightForward > weightRight) {
      ag.angle += randomSteer;
    } 
    else if (weightLeft > weightRight) {
      ag.angle += SENSOR_ANGLE + randomSteer;
    } 
    else if (weightRight > weightLeft) {
      ag.angle -= SENSOR_ANGLE + randomSteer;
    }
    else {
      ag.angle += (randf() - 0.5f) * 1.0f;
    }

    float newX = ag.x + cos(ag.angle) * MOVE_SPEED;
    float newY = ag.y + sin(ag.angle) * MOVE_SPEED;

    if (newX < 0) newX += WIDTH;
    if (newX >= WIDTH) newX -= WIDTH;
    if (newY < 0) newY += HEIGHT;
    if (newY >= HEIGHT) newY -= HEIGHT;
    
    ag.x = newX;
    ag.y = newY;

    int ix = wrapX((int)ag.x);
    int iy = wrapY((int)ag.y);
    
    trail[ag.species][ix][iy] += DEPOSIT_AMOUNT;
    if (trail[ag.species][ix][iy] > 100.0f) trail[ag.species][ix][iy] = 100.0f;
  }

  for (int s = 0; s < NUM_SPECIES; s++) {
    for (int y = 0; y < HEIGHT; y++) {
      for (int x = 0; x < WIDTH; x++) {
        trail[s][x][y] *= DECAY_RATE;
        if (trail[s][x][y] < 0.01f) trail[s][x][y] = 0.0f;
      }
    }

    static int frameCount = 0;
    if (frameCount % 3 == 0) {
      diffuseTrail(s);
    }
  }
  
  static int frameCount = 0;
  frameCount++;
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

      float maxTrail = 0.0f;
      int dominant = -1;
      
      for (int s = 0; s < NUM_SPECIES; s++) {
        if (trail[s][x][y] > maxTrail) {
          maxTrail = trail[s][x][y];
          dominant = s;
        }
      }
      
      uint16_t color = ST7735_BLACK;
      
      if (dominant >= 0 && maxTrail > 0.5f) {
        float intensity = (maxTrail > 100.0f) ? 1.0f : maxTrail / 100.0f;
        color = blendColors(ST7735_BLACK, coll[dominant], intensity);
      }
      
      for (int i = 0; i < TOTAL_AGENTS; i++) {
        int ax = wrapX((int)agents[i].x);
        int ay = wrapY((int)agents[i].y);
        if (ax == x && ay == y) {
          color = coll[agents[i].species];
          break;
        }
      }
      
      tft.pushColor(color);
    }
  }

}