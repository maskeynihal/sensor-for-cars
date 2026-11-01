// NOTE: superseded. This is the early OLED-only animation with NO sensors.
// The final build (DS18B20 water + DHT22 air + buzzer) is in
// ../water_monitor/water_monitor.ino  --  see ../docs/README.md
// Kept for reference / history only.

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_RESET     -1
#define SCREEN_ADDRESS 0x3C
Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ground baseline (y of the terrain under the wheels)
int16_t terrainY(int16_t worldX) {
  // rolling offroad hills: two sine waves added
  float h = 6.0 * sin(worldX * 0.06)
          + 3.0 * sin(worldX * 0.17 + 1.0);
  return 52 + (int16_t)h;
}

// draw the LC78: boxy wagon, long roof, snorkel, roof rack, round headlight
void drawLC78(int16_t x, int16_t y) {
  // y = ground level under the wheels; body sits above it

  int16_t bodyTop   = y - 18;   // roof line
  int16_t hoodTop   = y - 13;   // bonnet line (lower than roof)
  int16_t bodyBot   = y - 6;    // bottom of body / chassis
  int16_t left      = x;
  int16_t right     = x + 46;

  // --- chassis / body side profile ---
  // roof (cabin + cargo), flat and long
  display.drawLine(left+10, bodyTop, right-4, bodyTop, SH110X_WHITE);
  // windshield slope down to the bonnet
  display.drawLine(right-4, bodyTop, right, hoodTop, SH110X_WHITE);
  // bonnet (flat)
  display.drawLine(right, hoodTop, right+6, hoodTop, SH110X_WHITE);
  // front face down to bumper
  display.drawLine(right+6, hoodTop, right+6, bodyBot, SH110X_WHITE);
  // rear up from body to roof (vertical back - boxy wagon tail)
  display.drawLine(left+2, bodyBot, left+2, bodyTop+2, SH110X_WHITE);
  display.drawLine(left+2, bodyTop+2, left+10, bodyTop, SH110X_WHITE);
  // belt line / bottom of body
  display.drawLine(left+2, bodyBot, right+6, bodyBot, SH110X_WHITE);
  // A-pillar (windshield front edge marker)
  display.drawLine(left+10, bodyTop, left+10, hoodTop, SH110X_WHITE);

  // --- windows ---
  // cabin window (driver)
  display.drawRect(left+12, bodyTop+2, 10, 6, SH110X_WHITE);
  // rear cargo window
  display.drawRect(left+24, bodyTop+2, 8, 6, SH110X_WHITE);

  // --- roof rack (offroad touch) ---
  display.drawLine(left+10, bodyTop-3, right-6, bodyTop-3, SH110X_WHITE);
  display.drawLine(left+12, bodyTop-3, left+12, bodyTop, SH110X_WHITE);
  display.drawLine(left+20, bodyTop-3, left+20, bodyTop, SH110X_WHITE);
  display.drawLine(left+30, bodyTop-3, left+30, bodyTop, SH110X_WHITE);
  display.drawLine(right-8, bodyTop-3, right-8, bodyTop, SH110X_WHITE);

  // --- snorkel (runs up the front pillar) ---
  display.drawLine(right-3, bodyTop-2, right-3, hoodTop, SH110X_WHITE);
  display.drawLine(right-3, bodyTop-2, right-1, bodyTop-3, SH110X_WHITE);

  // --- round headlight (LC78 signature) ---
  display.fillCircle(right+4, hoodTop+3, 2, SH110X_WHITE);

  // --- wheels (big offroad tyres) ---
  int16_t wheelY = bodyBot + 4;
  display.fillCircle(left+10, wheelY, 5, SH110X_WHITE);   // rear
  display.fillCircle(right-2, wheelY, 5, SH110X_WHITE);   // front
  // hubs (black centre)
  display.fillCircle(left+10, wheelY, 2, SH110X_BLACK);
  display.fillCircle(right-2, wheelY, 2, SH110X_BLACK);
}

// a boulder obstacle sitting on the terrain
void drawBoulder(int16_t x, int16_t groundY) {
  display.fillCircle(x, groundY - 4, 6, SH110X_WHITE);
  display.fillCircle(x + 7, groundY - 2, 4, SH110X_WHITE);
  // carve the bottom flat into the ground
  display.fillRect(x - 8, groundY + 1, 24, 10, SH110X_BLACK);
}

void drawGround(int16_t scroll) {
  // draw the terrain line across the screen
  int16_t prevY = terrainY(scroll);
  for (int16_t sx = 0; sx < display.width(); sx++) {
    int16_t gy = terrainY(sx + scroll);
    display.drawLine(sx - 1, prevY, sx, gy, SH110X_WHITE);
    prevY = gy;
  }
}

void setup() {
  Serial.begin(9600);
  delay(300);

  if (!display.begin(SCREEN_ADDRESS, true)) {
    Serial.println(F("SH1106 allocation failed"));
    for (;;);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(22, 28);
  display.println(F("LC78 OFFROAD"));
  display.display();
  delay(1200);
}

void loop() {
  static int16_t scroll = 0;          // world scroll offset
  static int16_t boulderX = 128;      // obstacle position on screen
  int16_t carScreenX = 36;            // car stays near left-centre

  display.clearDisplay();

  // moving ground
  drawGround(scroll);

  // obstacle: a boulder rolling toward the car from the right
  int16_t boulderGround = terrainY(boulderX + scroll);
  drawBoulder(boulderX, boulderGround);

  // where is the ground right under the car?
  int16_t groundUnderCar = terrainY(carScreenX + 20 + scroll);

  // bump lift: if the boulder is near the car, hop over it
  int16_t lift = 0;
  int16_t dist = boulderX - (carScreenX + 20);
  if (dist > -14 && dist < 14) {
    // parabola hop centred on the boulder
    int16_t d = 14 - abs(dist);
    lift = d;                         // rises then falls
  }

  int16_t carY = groundUnderCar - lift;

  drawLC78(carScreenX, carY);

  // dust puffs behind the rear wheel
  int16_t dustX = carScreenX + 6;
  int16_t dustY = carY + 3;
  display.drawPixel(dustX - (scroll % 3), dustY, SH110X_WHITE);
  display.drawPixel(dustX - 4 - (scroll % 4), dustY - 2, SH110X_WHITE);
  display.drawPixel(dustX - 8 - (scroll % 5), dustY - 1, SH110X_WHITE);

  display.display();

  // advance the world
  scroll += 2;
  boulderX -= 3;
  if (boulderX < -16) {
    boulderX = display.width() + random(0, 60);   // respawn off-screen right
  }

  delay(40);
}