// =====================================================================
//  LC78 OFFROAD  --  Water + Air monitor  (Built by FriedRust)
// ---------------------------------------------------------------------
//  Board   : Arduino UNO R4 WiFi
//  Display : 1.3" SH1106 OLED, 128x64, I2C @ 0x3C  (SDA=A4, SCL=A5)
//  Sensors : DS18B20 water probe (OneWire) on D2  (+4.7k to 5V)
//            DHT22 air temp + humidity on D4
//  Buzzer  : ACTIVE buzzer on D3  (digitalWrite only -- no tone())
//
//  Buzzer logic (based on WATER temperature):
//    water  < 35 C          -> silent
//    water  35..40 C         -> beep on/off every 400 ms
//    water  > 40 C           -> ON continuously (steady alarm)
//
//  Serial log @ 9600 baud, every 2 s:
//    Water: 24.5C/76.1F   Air: 22.0C/71.6F  Hum:48%   Buzzer: BEEP
// =====================================================================

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define WATER_PIN 2            // DS18B20 water probe
#define DHT_PIN   4            // DHT22 air sensor
#define DHT_TYPE  DHT22
#define BUZZER_PIN 3
#define WARN_LIMIT 35.0        // beep from here
#define ALARM_LIMIT 40.0       // loud continuous above here

OneWire oneWire(WATER_PIN);
DallasTemperature waterSensor(&oneWire);
DHT dht(DHT_PIN, DHT_TYPE);

float waterC = 0, waterF = 0;
bool waterOK = false;
float airC = 0, airF = 0, humidity = 0;
bool airOK = false;

unsigned long lastWater = 0;
unsigned long lastAir = 0;

unsigned long lastBeep = 0;
bool beepOn = false;

int16_t terrainY(int16_t worldX) {
  float h = 6.0 * sin(worldX * 0.06) + 3.0 * sin(worldX * 0.17 + 1.0);
  return 52 + (int16_t)h;
}

void drawLC78(int16_t x, int16_t y) {
  int16_t bodyTop = y - 18;
  int16_t hoodTop = y - 13;
  int16_t bodyBot = y - 6;
  int16_t left = x;
  int16_t right = x + 46;

  display.drawLine(left+10, bodyTop, right-4, bodyTop, SH110X_WHITE);
  display.drawLine(right-4, bodyTop, right, hoodTop, SH110X_WHITE);
  display.drawLine(right, hoodTop, right+6, hoodTop, SH110X_WHITE);
  display.drawLine(right+6, hoodTop, right+6, bodyBot, SH110X_WHITE);
  display.drawLine(left+2, bodyBot, left+2, bodyTop+2, SH110X_WHITE);
  display.drawLine(left+2, bodyTop+2, left+10, bodyTop, SH110X_WHITE);
  display.drawLine(left+2, bodyBot, right+6, bodyBot, SH110X_WHITE);
  display.drawLine(left+10, bodyTop, left+10, hoodTop, SH110X_WHITE);

  display.drawRect(left+12, bodyTop+2, 10, 6, SH110X_WHITE);
  display.drawRect(left+24, bodyTop+2, 8, 6, SH110X_WHITE);

  display.drawLine(left+10, bodyTop-3, right-6, bodyTop-3, SH110X_WHITE);
  display.drawLine(left+12, bodyTop-3, left+12, bodyTop, SH110X_WHITE);
  display.drawLine(left+20, bodyTop-3, left+20, bodyTop, SH110X_WHITE);
  display.drawLine(left+30, bodyTop-3, left+30, bodyTop, SH110X_WHITE);
  display.drawLine(right-8, bodyTop-3, right-8, bodyTop, SH110X_WHITE);

  display.drawLine(right-3, bodyTop-2, right-3, hoodTop, SH110X_WHITE);
  display.drawLine(right-3, bodyTop-2, right-1, bodyTop-3, SH110X_WHITE);

  display.fillCircle(right+4, hoodTop+3, 2, SH110X_WHITE);

  int16_t wheelY = bodyBot + 4;
  display.fillCircle(left+10, wheelY, 5, SH110X_WHITE);
  display.fillCircle(right-2, wheelY, 5, SH110X_WHITE);
  display.fillCircle(left+10, wheelY, 2, SH110X_BLACK);
  display.fillCircle(right-2, wheelY, 2, SH110X_BLACK);
}

void drawBoulder(int16_t x, int16_t groundY) {
  display.fillCircle(x, groundY - 4, 6, SH110X_WHITE);
  display.fillCircle(x + 7, groundY - 2, 4, SH110X_WHITE);
  display.fillRect(x - 8, groundY + 1, 24, 10, SH110X_BLACK);
}

void drawGround(int16_t scroll) {
  int16_t prevY = terrainY(scroll);
  for (int16_t sx = 0; sx < display.width(); sx++) {
    int16_t gy = terrainY(sx + scroll);
    display.drawLine(sx - 1, prevY, sx, gy, SH110X_WHITE);
    prevY = gy;
  }
}

void drawReadout() {
  display.fillRect(0, 0, 62, 30, SH110X_BLACK);
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);

  display.setCursor(0, 2);
  if (!waterOK) display.print("W:--");
  else { display.print("W:"); display.print(waterC, 1); display.print((char)247); }

  display.setCursor(0, 12);
  if (!airOK) display.print("A:--");
  else { display.print("A:"); display.print(airC, 1); display.print((char)247); }

  display.setCursor(0, 22);
  if (!airOK) display.print("H:--");
  else { display.print("H:"); display.print(humidity, 0); display.print("%"); }
}

void drawBrand() {
  const char *txt = "FriedRust";
  int16_t w = strlen(txt) * 6;
  display.fillRect(128 - w - 2, 0, w + 2, 9, SH110X_BLACK);
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(128 - w, 1);
  display.print(txt);
}

void drawWarn(uint8_t frame, bool alarm) {
  if ((frame / 4) % 2) {
    display.fillRect(40, 26, 56, 14, SH110X_BLACK);
    display.drawRect(40, 26, 56, 14, SH110X_WHITE);
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(46, 30);
    display.print(alarm ? "W HOT!" : "WARM");
  }
}

void setup() {
  Serial.begin(9600);
  delay(300);

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  waterSensor.begin();
  waterSensor.setResolution(10);
  dht.begin();

  if (!display.begin(SCREEN_ADDRESS, true)) {
    Serial.println(F("SH1106 allocation failed"));
    for (;;);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(22, 24);
  display.println(F("LC78 OFFROAD"));
  display.setCursor(10, 40);
  display.println(F("Built by FriedRust"));
  display.display();
  delay(1500);

  waterSensor.requestTemperatures();
}

void loop() {
  static int16_t scroll = 0;
  static int16_t boulderX = 128;
  static uint8_t frame = 0;
  int16_t carScreenX = 36;

  frame++;

  // --- water temp (DS18B20), every 0.5s ---
  if (millis() - lastWater > 500) {
    float t = waterSensor.getTempCByIndex(0);
    if (t != DEVICE_DISCONNECTED_C) {
      waterC = t;
      waterF = t * 9.0 / 5.0 + 32.0;
      waterOK = true;
    } else waterOK = false;
    waterSensor.requestTemperatures();
    lastWater = millis();
  }

  // --- air + humidity (DHT22), every 2s, and log ---
  if (millis() - lastAir > 2000) {
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (!isnan(t) && !isnan(h)) {
      airC = t;
      airF = t * 9.0 / 5.0 + 32.0;
      humidity = h;
      airOK = true;
    } else airOK = false;
    lastAir = millis();

    Serial.print("Water: ");
    if (waterOK) { Serial.print(waterC, 1); Serial.print("C/"); Serial.print(waterF, 1); Serial.print("F"); }
    else Serial.print("--");
    Serial.print("   Air: ");
    if (airOK) { Serial.print(airC, 1); Serial.print("C/"); Serial.print(airF, 1); Serial.print("F  Hum:"); Serial.print(humidity, 0); Serial.print("%"); }
    else Serial.print("--");
    Serial.print("   Buzzer: ");
    if (!waterOK) Serial.println("OFF");
    else if (waterC > ALARM_LIMIT) Serial.println("LOUD-CONT");
    else if (waterC >= WARN_LIMIT) Serial.println("BEEP");
    else Serial.println("OFF");
  }

  // ----- buzzer: ACTIVE buzzer, digitalWrite only (no tone) -----
  bool alarm = (waterOK && waterC > ALARM_LIMIT);                             // >40 : loud continuous
  bool beep  = (waterOK && waterC >= WARN_LIMIT && waterC <= ALARM_LIMIT);    // 35-40 : beep

  if (alarm) {
    digitalWrite(BUZZER_PIN, HIGH);          // on, continuous
  } else if (beep) {
    if (millis() - lastBeep > 400) {         // toggle every 400ms
      beepOn = !beepOn;
      lastBeep = millis();
      digitalWrite(BUZZER_PIN, beepOn ? HIGH : LOW);
    }
  } else {
    digitalWrite(BUZZER_PIN, LOW);           // silent
    beepOn = false;
  }

  display.clearDisplay();

  drawGround(scroll);

  int16_t boulderGround = terrainY(boulderX + scroll);
  drawBoulder(boulderX, boulderGround);

  int16_t groundUnderCar = terrainY(carScreenX + 20 + scroll);
  int16_t lift = 0;
  int16_t dist = boulderX - (carScreenX + 20);
  if (dist > -14 && dist < 14) lift = 14 - abs(dist);
  int16_t carY = groundUnderCar - lift;
  drawLC78(carScreenX, carY);

  int16_t dustX = carScreenX + 6;
  int16_t dustY = carY + 3;
  display.drawPixel(dustX - (scroll % 3), dustY, SH110X_WHITE);
  display.drawPixel(dustX - 4 - (scroll % 4), dustY - 2, SH110X_WHITE);
  display.drawPixel(dustX - 8 - (scroll % 5), dustY - 1, SH110X_WHITE);

  drawReadout();
  drawBrand();
  if (beep || alarm) drawWarn(frame, alarm);

  display.display();

  scroll += 2;
  boulderX -= 3;
  if (boulderX < -16) boulderX = display.width() + random(0, 60);

  delay(40);
}
