// =====================================================================
//  hj60_coolant  --  HJ60 coolant temperature from the factory gauge sender
// ---------------------------------------------------------------------
//  Board   : ESP32 DevKit V1 (WROOM-32), powered 5 V from a 24 V
//            automotive front end (docs/adr/0009)
//  Inputs  : GPIO34 (ADC1) = gauge feed, temp gauge terminal E
//            GPIO35 (ADC1) = sender wire, temp gauge terminal F (YG)
//            Each through 120k/10k (13:1) + 100 nF + BAT54S clamp.
//  Display : 1.3" SH1106 OLED, I2C @ 0x3C (SDA=21, SCL=22)
//  Buzzer  : ACTIVE buzzer on GPIO25 via NPN transistor
//
//  Method (docs/gauge-tap-report.md, docs/adr/0008):
//    The gauge heater (Rg, E-F) and the sender (Rs) divide the pulsed
//    regulator feed, so k = Vs/Vfeed = Rs/(Rs+Rg) at every instant the
//    feed is on.  Rs = Rg*k/(1-k); Rs -> degC with a Beta fit through the
//    two FSM points (226 ohm @ 50 C, 26 ohm @ 115 C).
//
//  States : WAIT, OK, WARN (>=100 C), ALARM (>=105 C), 2 C hysteresis,
//           FAULT-NOFEED (no valid sample 5 s), FAULT-OPEN, FAULT-SHORT
//  Buzzer : OK silent / WARN beep 400 ms / ALARM steady /
//           FAULT double chirp every 5 s
//
//  Serial log @ 115200, once per second:
//    Coolant: 92.4C  Rs:52.1ohm  k:0.676  Feed:11.7V  State:OK
//    Coolant: --  Rs:--  k:--  Feed:--  State:FAULT-NOFEED
// =====================================================================

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <math.h>

#define FEED_PIN    34
#define SENDER_PIN  35
#define BUZZER_PIN  25

#define SCREEN_ADDRESS 0x3C
Adafruit_SH1106G display(128, 64, &Wire, -1);

// ---- hardware constants (measure on the truck: report section 6) ----
#define DIVIDER_RATIO 13.0      // (120k + 10k) / 10k
#define GAUGE_R_OHMS  25.0      // temp gauge E-F, FSM "STD 25 ohm" (M1)
#define SENDER_R0     226.0     // ohm at SENDER_T0_C (FSM 13-74)
#define SENDER_T0_C   50.0
#define SENDER_R1     26.0      // ohm at SENDER_T1_C (FSM 13-74)
#define SENDER_T1_C   115.0

// ---- thresholds ----
#define WARN_C   100.0
#define ALARM_C  105.0
#define HYST_C   2.0

// ---- sampling / fault detection ----
#define FEED_ON_V       3.0     // feed at the tap must be above this
#define EDGE_TOL        0.03    // feed may move <=3% across one pair
#define K_OPEN          0.998   // Rs > ~12 kohm: sender wire open
#define RS_SHORT_OHMS   5.0     // sender shorted to ground
#define SAMPLE_MS       10
#define WINDOW_MS       1000
#define MIN_PAIRS       5
#define NOFEED_MS       5000
#define MAX_PAIRS       (WINDOW_MS / SAMPLE_MS + 1)

enum State { ST_WAIT, ST_OK, ST_WARN, ST_ALARM, ST_NOFEED, ST_OPEN, ST_SHORT };

float kSamples[MAX_PAIRS];
int kCount = 0;
float feedSum = 0;

float coolantC = NAN, senderOhms = NAN, ratioK = NAN, feedV = NAN;
State state = ST_WAIT;
bool displayOk = false;

unsigned long lastSample = 0;
unsigned long windowStart = 0;
unsigned long lastValid = 0;
unsigned long lastBeep = 0;
unsigned long faultSince = 0;
bool beepOn = false;

float tapVolts(uint8_t pin) {
  return analogReadMilliVolts(pin) * DIVIDER_RATIO / 1000.0;
}

float senderBeta() {
  const float t0 = SENDER_T0_C + 273.15, t1 = SENDER_T1_C + 273.15;
  return log(SENDER_R0 / SENDER_R1) / (1.0 / t0 - 1.0 / t1);
}

float ohmsToC(float rs) {
  float invT = 1.0 / (SENDER_T0_C + 273.15) + log(rs / SENDER_R0) / senderBeta();
  return 1.0 / invT - 273.15;
}

// One feed/sender/feed triple. Kept only if the feed is on and steady.
void samplePair() {
  float f1 = tapVolts(FEED_PIN);
  float s = tapVolts(SENDER_PIN);
  float f2 = tapVolts(FEED_PIN);
  if (f1 < FEED_ON_V || f2 < FEED_ON_V) return;
  if (fabs(f1 - f2) > EDGE_TOL * max(f1, f2)) return;
  if (kCount >= MAX_PAIRS) return;
  float f = (f1 + f2) / 2.0;
  kSamples[kCount++] = s / f;
  feedSum += f;
}

float medianK() {
  for (int i = 1; i < kCount; i++) {
    float v = kSamples[i];
    int j = i - 1;
    while (j >= 0 && kSamples[j] > v) { kSamples[j + 1] = kSamples[j]; j--; }
    kSamples[j + 1] = v;
  }
  return kSamples[kCount / 2];
}

bool isFault(State s) { return s == ST_NOFEED || s == ST_OPEN || s == ST_SHORT; }

State tempState(float c, State prev) {
  if (c >= ALARM_C) return ST_ALARM;
  if (prev == ST_ALARM && c >= ALARM_C - HYST_C) return ST_ALARM;
  if (c >= WARN_C) return ST_WARN;
  if ((prev == ST_ALARM || prev == ST_WARN) && c >= WARN_C - HYST_C) return ST_WARN;
  return ST_OK;
}

void setState(State next) {
  if (isFault(next) && !isFault(state)) faultSince = millis();
  state = next;
}

void evaluateWindow() {
  if (kCount >= MIN_PAIRS) {
    lastValid = millis();
    ratioK = medianK();
    feedV = feedSum / kCount;
    if (ratioK >= K_OPEN) {
      senderOhms = NAN; coolantC = NAN;
      setState(ST_OPEN);
    } else {
      senderOhms = GAUGE_R_OHMS * ratioK / (1.0 - ratioK);
      if (senderOhms < RS_SHORT_OHMS) {
        coolantC = NAN;
        setState(ST_SHORT);
      } else {
        coolantC = ohmsToC(senderOhms);
        setState(tempState(coolantC, isFault(state) ? ST_OK : state));
      }
    }
  } else if (millis() - lastValid >= NOFEED_MS) {
    // Feed off-phases are expected (pulsing regulator): hold the last
    // reading until there has been no valid pair for NOFEED_MS.
    coolantC = senderOhms = ratioK = feedV = NAN;
    setState(ST_NOFEED);
  }
  kCount = 0;
  feedSum = 0;
}

const char *stateName() {
  switch (state) {
    case ST_WAIT:   return "WAIT";
    case ST_OK:     return "OK";
    case ST_WARN:   return "WARN";
    case ST_ALARM:  return "ALARM";
    case ST_NOFEED: return "FAULT-NOFEED";
    case ST_OPEN:   return "FAULT-OPEN";
    case ST_SHORT:  return "FAULT-SHORT";
  }
  return "?";
}

void printOr(const char *label, float v, int digits, const char *unit) {
  Serial.print(label);
  if (isnan(v)) Serial.print(F("--"));
  else { Serial.print(v, digits); Serial.print(unit); }
}

void logLine() {
  printOr("Coolant: ", coolantC, 1, "C");
  printOr("  Rs:", senderOhms, 1, "ohm");
  printOr("  k:", ratioK, 3, "");
  printOr("  Feed:", feedV, 1, "V");
  Serial.print(F("  State:"));
  Serial.println(stateName());
}

void drawScreen() {
  if (!displayOk) return;
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(F("COOLANT"));
  display.setTextSize(3);
  display.setCursor(0, 16);
  if (isnan(coolantC)) display.print(F("--"));
  else { display.print((int)lround(coolantC)); display.print(F("C")); }
  display.setTextSize(1);
  display.setCursor(0, 54);
  display.print(stateName());
  display.display();
}

void driveBuzzer() {
  unsigned long now = millis();
  if (state == ST_ALARM) {
    digitalWrite(BUZZER_PIN, HIGH);
  } else if (state == ST_WARN) {
    if (now - lastBeep >= 400) {
      lastBeep = now;
      beepOn = !beepOn;
      digitalWrite(BUZZER_PIN, beepOn ? HIGH : LOW);
    }
  } else if (isFault(state)) {
    unsigned long phase = (now - faultSince) % 5000;
    bool on = phase < 100 || (phase >= 200 && phase < 300);
    digitalWrite(BUZZER_PIN, on ? HIGH : LOW);
  } else {
    digitalWrite(BUZZER_PIN, LOW);
    beepOn = false;
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  analogReadResolution(12);
  analogSetPinAttenuation(FEED_PIN, ADC_11db);
  analogSetPinAttenuation(SENDER_PIN, ADC_11db);

  Wire.begin();
  // A missing display must not stop the alarm: log it and carry on.
  displayOk = display.begin(SCREEN_ADDRESS, true);
  if (!displayOk) Serial.println(F("OLED not found, continuing without display"));

  Serial.println(F("hj60_coolant boot ok"));
  lastValid = millis();
  windowStart = millis();
  drawScreen();
}

void loop() {
  unsigned long now = millis();
  if (now - lastSample >= SAMPLE_MS) {
    lastSample = now;
    samplePair();
  }
  if (now - windowStart >= WINDOW_MS) {
    windowStart = now;
    evaluateWindow();
    logLine();
    drawScreen();
  }
  driveBuzzer();
}
