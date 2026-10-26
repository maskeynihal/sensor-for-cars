#include <TFT_eSPI.h>
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();

#define W 480
#define H 320
#define SKY  TFT_SKYBLUE

// ground height under a given world x
int16_t terrainY(int32_t worldX) {
  float h = 28.0 * sin(worldX * 0.012)
          + 14.0 * sin(worldX * 0.035 + 1.0);
  return 250 + (int16_t)h;
}

// repaint background inside a rectangle only (sky + dirt + ground line)
void restoreBG(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int32_t scroll) {
  if (x0 < 0) x0 = 0;
  if (x1 > W) x1 = W;
  if (y0 < 0) y0 = 0;
  if (y1 > H) y1 = H;
  for (int16_t sx = x0; sx < x1; sx++) {
    int16_t gy = terrainY(sx + scroll);
    int16_t skyTop = y0;
    int16_t skyBot = min((int)y1, (int)gy);
    if (skyBot > skyTop) tft.drawFastVLine(sx, skyTop, skyBot - skyTop, SKY);
    int16_t dirtTop = max((int)y0, (int)gy);
    int16_t dirtBot = y1;
    if (dirtBot > dirtTop) tft.drawFastVLine(sx, dirtTop, dirtBot - dirtTop, TFT_MAROON);
    if (gy >= y0 && gy < y1) tft.drawPixel(sx, gy, TFT_ORANGE);
  }
}

void drawLC78(int16_t x, int16_t y) {
  int16_t bodyTop = y - 90;
  int16_t hoodTop = y - 65;
  int16_t bodyBot = y - 30;
  int16_t left    = x;
  int16_t right   = x + 230;

  tft.fillRect(left+10, bodyTop, (right-10)-(left+10), bodyBot-bodyTop, TFT_WHITE);
  tft.fillRect(right-10, hoodTop, 40, bodyBot-hoodTop, TFT_WHITE);
  tft.drawRect(left+10, bodyTop, (right-10)-(left+10), bodyBot-bodyTop, TFT_BLACK);
  tft.drawRect(right-10, hoodTop, 40, bodyBot-hoodTop, TFT_BLACK);

  tft.fillRect(left+30, bodyTop+10, 55, 35, TFT_CYAN);
  tft.fillRect(left+95, bodyTop+10, 45, 35, TFT_CYAN);
  tft.drawRect(left+30, bodyTop+10, 55, 35, TFT_BLACK);
  tft.drawRect(left+95, bodyTop+10, 45, 35, TFT_BLACK);

  tft.fillRect(left+20, bodyTop-12, 150, 6, TFT_DARKGREY);
  for (int16_t rx = left+30; rx < left+170; rx += 30)
    tft.drawFastVLine(rx, bodyTop-12, 12, TFT_DARKGREY);

  tft.fillRect(right-14, bodyTop-10, 7, hoodTop-(bodyTop-10), TFT_DARKGREY);

  tft.fillCircle(right+28, hoodTop+18, 8, TFT_YELLOW);
  tft.drawCircle(right+28, hoodTop+18, 8, TFT_BLACK);

  int16_t wheelY = bodyBot + 18;
  tft.fillCircle(left+45, wheelY, 26, TFT_BLACK);
  tft.fillCircle(right-20, wheelY, 26, TFT_BLACK);
  tft.fillCircle(left+45, wheelY, 11, TFT_DARKGREY);
  tft.fillCircle(right-20, wheelY, 11, TFT_DARKGREY);
}

void drawBoulder(int16_t x, int16_t groundY) {
  tft.fillCircle(x, groundY - 20, 28, TFT_DARKGREY);
  tft.fillCircle(x + 30, groundY - 10, 18, TFT_DARKGREY);
  tft.drawCircle(x, groundY - 20, 28, TFT_BLACK);
}

// car bounding box around anchor (x, y)
void carBox(int16_t x, int16_t y, int16_t &x0, int16_t &y0, int16_t &x1, int16_t &y1) {
  x0 = x - 2;
  x1 = x + 230 + 40;
  y0 = (y - 90) - 14;
  y1 = (y - 30) + 18 + 28;
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

  // paint full background ONCE
  for (int16_t sx = 0; sx < W; sx++) {
    int16_t gy = terrainY(sx);
    tft.drawFastVLine(sx, 0, gy, SKY);
    tft.drawFastVLine(sx, gy, H - gy, TFT_MAROON);
    tft.drawPixel(sx, gy, TFT_ORANGE);
  }
}

void loop() {
  static int32_t scroll = 0;
  static bool first = true;
  static int16_t lastCarY;
  static int16_t lastBoulderX, lastBoulderGY;

  static int16_t boulderX = W;
  int16_t carScreenX = 120;

  int16_t groundUnderCar = terrainY(carScreenX + 110 + scroll);
  int16_t lift = 0;
  int16_t dist = boulderX - (carScreenX + 110);
  if (dist > -70 && dist < 70) lift = 70 - abs(dist);
  int16_t carY = groundUnderCar - lift;
  int16_t boulderGY = terrainY(boulderX + scroll);

  // erase boulder's old spot
  if (!first) {
    restoreBG(lastBoulderX - 30, lastBoulderGY - 50,
              lastBoulderX + 50, lastBoulderGY + 5, scroll);
  }

  // erase only the strip the car vacated on a hop
  if (!first && carY != lastCarY) {
    int16_t x0, y0, x1, y1;
    carBox(carScreenX, lastCarY, x0, y0, x1, y1);
    if (carY > lastCarY) {
      restoreBG(x0, y0, x1, y0 + (carY - lastCarY) + 2, scroll);
    } else {
      restoreBG(x0, y1 - (lastCarY - carY) - 2, x1, y1, scroll);
    }
  }

  // draw boulder, then car over itself (no blank flash)
  drawBoulder(boulderX, boulderGY);
  drawLC78(carScreenX, carY);

  first = false;
  lastCarY = carY;
  lastBoulderX = boulderX;
  lastBoulderGY = boulderGY;

  scroll += 4;
  boulderX -= 6;
  if (boulderX < -40) boulderX = W + random(0, 200);

  delay(20);
}