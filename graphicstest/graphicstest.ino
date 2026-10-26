#include <TFT_eSPI.h>
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite car = TFT_eSprite(&tft);   // small off-screen buffer for the car

#define W 480
#define H 320
#define SKY  TFT_SKYBLUE

#define CARW 300
#define CARH 150

int16_t terrainY(int32_t worldX) {
  float h = 28.0 * sin(worldX * 0.012)
          + 14.0 * sin(worldX * 0.035 + 1.0);
  return 250 + (int16_t)h;
}

// draw the truck INTO the sprite, truck's ground line at local y
void drawLC78intoSprite(int16_t y) {
  int16_t x = 10;
  int16_t bodyTop = y - 90;
  int16_t hoodTop = y - 65;
  int16_t bodyBot = y - 30;
  int16_t left    = x;
  int16_t right   = x + 230;

  car.fillRect(left+10, bodyTop, (right-10)-(left+10), bodyBot-bodyTop, TFT_WHITE);
  car.fillRect(right-10, hoodTop, 40, bodyBot-hoodTop, TFT_WHITE);
  car.drawRect(left+10, bodyTop, (right-10)-(left+10), bodyBot-bodyTop, TFT_BLACK);
  car.drawRect(right-10, hoodTop, 40, bodyBot-hoodTop, TFT_BLACK);

  car.fillRect(left+30, bodyTop+10, 55, 35, TFT_CYAN);
  car.fillRect(left+95, bodyTop+10, 45, 35, TFT_CYAN);
  car.drawRect(left+30, bodyTop+10, 55, 35, TFT_BLACK);
  car.drawRect(left+95, bodyTop+10, 45, 35, TFT_BLACK);

  car.fillRect(left+20, bodyTop-12, 150, 6, TFT_DARKGREY);
  for (int16_t rx = left+30; rx < left+170; rx += 30)
    car.drawFastVLine(rx, bodyTop-12, 12, TFT_DARKGREY);

  car.fillRect(right-14, bodyTop-10, 7, hoodTop-(bodyTop-10), TFT_DARKGREY);

  car.fillCircle(right+28, hoodTop+18, 8, TFT_YELLOW);
  car.drawCircle(right+28, hoodTop+18, 8, TFT_BLACK);

  int16_t wheelY = bodyBot + 18;
  car.fillCircle(left+45, wheelY, 26, TFT_BLACK);
  car.fillCircle(right-20, wheelY, 26, TFT_BLACK);
  car.fillCircle(left+45, wheelY, 11, TFT_DARKGREY);
  car.fillCircle(right-20, wheelY, 11, TFT_DARKGREY);
}

void drawBoulder(int16_t x, int16_t groundY) {
  tft.fillCircle(x, groundY - 20, 28, TFT_DARKGREY);
  tft.fillCircle(x + 30, groundY - 10, 18, TFT_DARKGREY);
  tft.drawCircle(x, groundY - 20, 28, TFT_BLACK);
}

void paintBackground() {
  for (int16_t sx = 0; sx < W; sx++) {
    int16_t gy = terrainY(sx);
    tft.drawFastVLine(sx, 0, gy, SKY);
    tft.drawFastVLine(sx, gy, H - gy, TFT_MAROON);
    tft.drawPixel(sx, gy, TFT_ORANGE);
  }
}

void setup() {
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(SKY);

  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(3);
  tft.setCursor(150, 140);
  tft.print("LC78 OFFROAD");
  delay(1200);

  paintBackground();

  // 4-bit sprite fits the UNO R4 RAM (300x150 / 2 = ~22 KB)
  car.setColorDepth(4);
  car.createSprite(CARW, CARH);

  // explicit 16-color palette (resolves the ambiguous createPalette call)
  static uint16_t palette[16] = {
    TFT_SKYBLUE,  // 0 - sky / transparent
    TFT_WHITE,    // 1 - body
    TFT_BLACK,    // 2 - outlines
    TFT_CYAN,     // 3 - windows
    TFT_DARKGREY, // 4 - rack / snorkel / hubs
    TFT_YELLOW,   // 5 - headlight
    TFT_MAROON,   // 6 - dirt
    TFT_ORANGE,   // 7 - ground line
    TFT_RED, TFT_GREEN, TFT_BLUE, TFT_LIGHTGREY,
    TFT_NAVY, TFT_DARKGREEN, TFT_OLIVE, TFT_SILVER
  };
  car.createPalette(palette, 16);
}

void loop() {
  static int16_t carScreenX = 120;
  static int16_t boulderX = W;
  static int16_t lastCarTop = -1;
  static int16_t lastBoulderX = -999, lastBoulderGY = 0;

  // gentle bounce
  static float t = 0;
  t += 0.15;
  int16_t baseY = terrainY(carScreenX + 110);
  int16_t lift = (int16_t)(18 * fabs(sin(t)));
  int16_t carY = baseY - lift;

  int16_t carTop = carY - 120;
  if (carTop < 0) carTop = 0;

  // build the car sprite
  car.fillSprite(SKY);
  drawLC78intoSprite(120);

  // erase the band the car vacated when it moved vertically
  if (lastCarTop >= 0 && lastCarTop != carTop) {
    int16_t y0 = min(lastCarTop, carTop);
    int16_t y1 = max(lastCarTop, carTop) + CARH;
    for (int16_t sx = carScreenX; sx < carScreenX + CARW && sx < W; sx++) {
      int16_t gy = terrainY(sx);
      for (int16_t sy = y0; sy < y1 && sy < H; sy++) {
        tft.drawPixel(sx, sy, sy < gy ? SKY : TFT_MAROON);
      }
    }
  }

  // push the car, sky-blue is transparent
  car.pushSprite(carScreenX, carTop, SKY);

  // boulder: erase old spot, draw new
  if (lastBoulderX > -900) {
    for (int16_t sx = lastBoulderX - 30; sx < lastBoulderX + 50; sx++) {
      if (sx < 0 || sx >= W) continue;
      int16_t gy = terrainY(sx);
      for (int16_t sy = lastBoulderGY - 50; sy < lastBoulderGY + 5; sy++) {
        if (sy < 0 || sy >= H) continue;
        tft.drawPixel(sx, sy, sy < gy ? SKY : TFT_MAROON);
      }
    }
  }
  int16_t boulderGY = terrainY(boulderX);
  drawBoulder(boulderX, boulderGY);

  // save state
  lastBoulderX = boulderX;
  lastBoulderGY = boulderGY;
  lastCarTop = carTop;

  boulderX -= 6;
  if (boulderX < -40) boulderX = W + random(0, 200);

  delay(30);
}