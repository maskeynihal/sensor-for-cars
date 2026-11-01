// =====================================================================
//  hj60_coolant  --  HJ60 coolant temperature + FriedRust TFT dashboard
// ---------------------------------------------------------------------
//  Board   : ESP32-S3 DevKitC-1 (N16R8), powered 5 V from a 12 V
//            automotive front end (docs/adr/0009, docs/adr/0012)
//            GPIO26-37 are flash/PSRAM on this module: do not use them.
//  Display : ILI9488 3.5" 480x320 SPI TFT (docs/adr/0010)
//            SCK=12 MOSI=11 CS=10 DC=9 RST=8 backlight=18
//  Buzzer  : ACTIVE buzzer on GPIO17 via NPN transistor
//  Button  : GPIO15 to ground (next screen)
//  Power   : USB (car USB adapter). ESP32 GND shares the truck ground.
//  Battery : optional, GPIO6 via 68k/10k (7.8:1) from 12 V. Set
//            batterySense = true only when that divider is wired.
//
//  SENDER MODE (set senderMode below before you flash):
//   A = passive taps, dash gauge keeps working (docs/adr/0008)
//       GPIO4 = gauge feed E, GPIO5 = sender wire F, each 68k/10k.
//       k = Vs/Vfeed = Rs/(Rs+Rg) while the regulator is on.
//   B = direct: YG wire DISCONNECTED from the gauge, wired to the ESP32.
//       THE DASH GAUGE DOES NOT WORK IN MODE B.
//       GPIO7 drives the sender directly: GPIO7 -> Rref 220 ohm -> YG
//       wire -> sender -> engine ground (~13 mA, only while measuring).
//       GPIO4 = Vtop (GPIO7 side of Rref), GPIO5 = Vs, each through a
//       matched 5.6k/12k divider (keeps both pins below ~2.2 V, where the
//       ADC is linear, so an open wire is not read as "cold"). The Vs
//       divider (17.6k) is in parallel with the sender; the firmware
//       removes it from the result (VS_LOAD_OHMS).
//       No diodes or transistors. A short from the YG wire to 12 V can
//       damage GPIO7: add a Schottky in series later if you can.
//       Rs = Rref*Vs/(Vtop-Vs). Vs is also read with the excitation
//       off: voltage there means the gauge or a battery wire is still
//       on the sender wire (FAULT-BACKFEED).
//   Rs -> degC with a Beta fit through the FSM points 226 ohm @ 50 C
//   and 26 ohm @ 115 C (calibrate: report step M4).
//
//  States : WAIT, OK, WARN (>=100 C), ALARM (>=105 C), 2 C hysteresis,
//           FAULT-NOFEED (A) / FAULT-NOEXCITE (B), FAULT-OPEN,
//           FAULT-SHORT, FAULT-BACKFEED (B)
//  Buzzer : OK silent / WARN beep 400 ms / ALARM steady /
//           FAULT double chirp every 5 s
//  Screens: BOOT -> CHECK -> MAIN / GRID / WATER (button cycles).
//           WARN and ALARM switch to WATER. RPM and SPEED are DEMO.
//
//  Serial log @ 115200. At boot, a Config line. Then once per second a Raw
//  line (pin millivolts and voltages, for checking the wiring) and the
//  state line:
//    Config: Mode:B  Rref:220ohm  Div:1.467  Load:17600ohm  Warn:100C  Alarm:105C  BatterySense:0
//    Raw: GPIO4:2182mV  GPIO5:261mV  Vtop:3.20V  Vs:0.38V  Rp:33.6ohm  Pairs:10  OffHits:0/10
//    Coolant: 92.4C  Rs:52.1ohm  k:0.676  Feed:11.7V  Batt:12.9V  Mode:A  State:OK
//    Coolant: 92.4C  Rs:52.1ohm  k:0.343  Vtop:3.00V  Batt:12.9V  Mode:B  State:OK
//    Coolant: --  Rs:--  k:--  Feed:--  Batt:12.9V  Mode:A  State:FAULT-NOFEED
// =====================================================================

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <DIYables_TFT_SPI.h>
#include <math.h>
#include "boot_art.h"

// ---- sender mode: change before you flash ----
enum SenderMode { SENDER_MODE_A, SENDER_MODE_B };
SenderMode senderMode = SENDER_MODE_B;
// ---- battery voltage on GPIO6: true only if the 68k/10k divider from 12 V is wired ----
bool batterySense = false;

// ---- pins ----
// ESP32-S3: ADC1 is GPIO1-10. SPI uses the default FSPI pins SCK=12, MOSI=11.
#define FEED_PIN      4   // A: gauge feed E  / B: Vtop   (ADC1_CH3)
#define SENDER_PIN    5   // A: sender wire F / B: Vs     (ADC1_CH4)
#define BATT_PIN      6   // (ADC1_CH5)
#define EXCITE_PIN    7   // B: excitation switch, HIGH = on
#define BUZZER_PIN   17
#define BUTTON_PIN   15
#define TFT_CS       10
#define TFT_DC        9
#define TFT_RST       8
#define TFT_BL       18

DIYables_ILI9488_SPI tft(320, 480, TFT_CS, TFT_DC, TFT_RST);

// ---- hardware constants (measure on the truck: report section 6) ----
#define DIVIDER_RATIO  7.8      // A taps and battery: (68k + 10k) / 10k
#define VTOP_RATIO     (17.6 / 12.0)  // B: Vtop through 5.6k/12k
#define VS_RATIO       (17.6 / 12.0)  // B: Vs through 5.6k/12k (matched to Vtop)
#define VS_LOAD_OHMS   17600.0  // B: the Vs divider, in parallel with the sender
#define GAUGE_R_OHMS   25.0     // A: temp gauge E-F, FSM "STD 25 ohm" (M1)
#define REF_R_OHMS     220.0    // B: Rref (1% or better)
#define SENDER_R0      226.0    // ohm at SENDER_T0_C (FSM 13-74)
#define SENDER_T0_C    50.0
#define SENDER_R1      26.0     // ohm at SENDER_T1_C (FSM 13-74)
#define SENDER_T1_C    115.0
#define ADC_OFFSET_MV  0        // subtract from every ADC reading; set from step M5

// ---- thresholds ----
#define WARN_C   100.0
#define ALARM_C  105.0
#define HYST_C   2.0

// ---- sampling / fault detection ----
#define FEED_ON_V       3.0     // A: feed at the tap must be above this
#define EDGE_TOL        0.03    // A: feed may move <=3% across one pair
#define K_OPEN_A        0.998   // A: Rs > ~12 kohm
#define K_OPEN_B        0.97    // B: open wire gives k = 17.6k/(17.6k+220) = 0.988
#define RS_SHORT_OHMS   5.0
#define EXCITE_MIN_V    2.0     // B: Vtop below this = excitation failed
#define BACKFEED_V      0.6     // B: Vs with excitation off above this = fault (ADC floor ~0.15 V x1.47)
#define EXCITE_PERIOD_MS 100    // B: one measurement every 100 ms
#define EXCITE_SETTLE_MS 2      // B: excitation on -> read
#define SAMPLE_MS       10      // A: one pair every 10 ms
#define WINDOW_MS       1000
#define MIN_PAIRS       5
#define NOFEED_MS       5000
#define MAX_PAIRS       (WINDOW_MS / SAMPLE_MS + 1)

// ---- UI timing ----
#define BOOT_MS     1500
#define CHECK_MS    3000
#define RESULT_MS   1500
#define UI_MS       500
#define DEBOUNCE_MS 30

enum State { ST_WAIT, ST_OK, ST_WARN, ST_ALARM, ST_NOFEED, ST_OPEN, ST_SHORT, ST_BACKFEED };
enum Screen { SCR_BOOT, SCR_CHECK, SCR_MAIN, SCR_GRID, SCR_WATER };

float kSamples[MAX_PAIRS];
int kCount = 0;
float refSum = 0;          // A: feed volts, B: Vtop volts
float sigSum = 0;          // A: sender wire volts, B: Vs volts
float rawRefV = NAN, rawSigV = NAN, rawRp = NAN;   // last window, for the Raw log line
int rawPairs = 0, rawOffHits = 0, rawOffSamples = 0;
int backfeedHits = 0, offSamples = 0;
float battSum = 0;
int battCount = 0;

float coolantC = NAN, senderOhms = NAN, ratioK = NAN, refV = NAN, battV = NAN;
State state = ST_WAIT;

unsigned long lastSample = 0, lastBatt = 0, windowStart = 0, lastValid = 0;
unsigned long lastBeep = 0, faultSince = 0;
bool beepOn = false;

// B excitation state machine
bool exciteOn = false;
unsigned long exciteStart = 0, lastExcite = 0;

// UI
Screen screen = SCR_BOOT, userScreen = SCR_MAIN, drawnScreen = SCR_BOOT;
bool screenDrawn = false, flashOn = false;
unsigned long screenSince = 0, lastUi = 0, lastBtnChange = 0;
int lastBtn = HIGH;
float demoRpm = 0, demoSpd = 0;
typedef char Txt[28];
Txt shown[12];             // last text drawn in each value slot

// ===================================================================
//  Conversion
// ===================================================================
float senderBeta() {
  const float t0 = SENDER_T0_C + 273.15, t1 = SENDER_T1_C + 273.15;
  return log(SENDER_R0 / SENDER_R1) / (1.0 / t0 - 1.0 / t1);
}

float ohmsToC(float rs) {
  float invT = 1.0 / (SENDER_T0_C + 273.15) + log(rs / SENDER_R0) / senderBeta();
  return 1.0 / invT - 273.15;
}

float pinVolts(uint8_t pin, float ratio) {
  int mv = (int)analogReadMilliVolts(pin) - ADC_OFFSET_MV;
  return max(0, mv) * ratio / 1000.0;
}

// ===================================================================
//  Sampling
// ===================================================================
void addPair(float k, float ref, float sig) {
  if (kCount >= MAX_PAIRS) return;
  kSamples[kCount++] = k;
  refSum += ref;
  sigSum += sig;
}

// A: one feed/sender/feed triple, kept only if the feed is on and steady.
void samplePairA() {
  float f1 = pinVolts(FEED_PIN, DIVIDER_RATIO);
  float s = pinVolts(SENDER_PIN, DIVIDER_RATIO);
  float f2 = pinVolts(FEED_PIN, DIVIDER_RATIO);
  if (f1 < FEED_ON_V || f2 < FEED_ON_V) return;
  if (fabs(f1 - f2) > EDGE_TOL * max(f1, f2)) return;
  float f = (f1 + f2) / 2.0;
  addPair(s / f, f, s);
}

// B: off-check, excitation on, settle, read, excitation off. Non-blocking.
void serviceExcitationB() {
  unsigned long now = millis();
  if (!exciteOn && now - lastExcite >= EXCITE_PERIOD_MS) {
    lastExcite = now;
    offSamples++;
    if (pinVolts(SENDER_PIN, VS_RATIO) > BACKFEED_V) backfeedHits++;
    digitalWrite(EXCITE_PIN, HIGH);
    exciteOn = true;
    exciteStart = now;
  } else if (exciteOn && now - exciteStart >= EXCITE_SETTLE_MS) {
    float vtop = pinVolts(FEED_PIN, VTOP_RATIO);
    float vs = pinVolts(SENDER_PIN, VS_RATIO);
    digitalWrite(EXCITE_PIN, LOW);
    exciteOn = false;
    if (vtop >= EXCITE_MIN_V) addPair(vs / vtop, vtop, vs);
  }
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

// ===================================================================
//  State
// ===================================================================
bool isFault(State s) { return s == ST_NOFEED || s == ST_OPEN || s == ST_SHORT || s == ST_BACKFEED; }
bool isAlert(State s) { return s == ST_WARN || s == ST_ALARM; }

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

void clearReading() { coolantC = senderOhms = ratioK = NAN; }

void evaluateWindow() {
  rawRp = NAN;
  if (battCount > 0) battV = battSum / battCount;
  battSum = 0; battCount = 0;

  bool backfeed = senderMode == SENDER_MODE_B && offSamples > 0 && backfeedHits * 2 >= offSamples;
  if (backfeed) {
    clearReading(); refV = NAN;
    setState(ST_BACKFEED);
  } else if (kCount >= MIN_PAIRS) {
    lastValid = millis();
    ratioK = medianK();
    refV = refSum / kCount;
    float rRef = senderMode == SENDER_MODE_A ? GAUGE_R_OHMS : REF_R_OHMS;
    float kOpen = senderMode == SENDER_MODE_A ? K_OPEN_A : K_OPEN_B;
    float rp = ratioK < 1.0 ? rRef * ratioK / (1.0 - ratioK) : INFINITY;
    rawRp = rp;
    // B: rp is the sender in parallel with the Vs divider. Remove the divider.
    if (senderMode == SENDER_MODE_B && rp < VS_LOAD_OHMS) rp = rp * VS_LOAD_OHMS / (VS_LOAD_OHMS - rp);
    else if (senderMode == SENDER_MODE_B) rp = INFINITY;
    if (ratioK >= kOpen || isinf(rp)) {
      senderOhms = NAN; coolantC = NAN;
      setState(ST_OPEN);
    } else {
      senderOhms = rp;
      if (senderOhms < RS_SHORT_OHMS) {
        coolantC = NAN;
        setState(ST_SHORT);
      } else {
        coolantC = ohmsToC(senderOhms);
        setState(tempState(coolantC, isFault(state) ? ST_OK : state));
      }
    }
  } else if (millis() - lastValid >= NOFEED_MS) {
    // A: regulator off-phases are expected, so hold the last reading
    // until there has been no valid pair for NOFEED_MS.
    clearReading(); refV = NAN;
    setState(ST_NOFEED);
  }
  rawPairs = kCount;
  rawRefV = kCount ? refSum / kCount : NAN;
  rawSigV = kCount ? sigSum / kCount : NAN;
  rawOffHits = backfeedHits; rawOffSamples = offSamples;
  kCount = 0; refSum = 0; sigSum = 0;
  backfeedHits = 0; offSamples = 0;
}

const char *stateName() {
  switch (state) {
    case ST_WAIT:     return "WAIT";
    case ST_OK:       return "OK";
    case ST_WARN:     return "WARN";
    case ST_ALARM:    return "ALARM";
    case ST_NOFEED:   return senderMode == SENDER_MODE_A ? "FAULT-NOFEED" : "FAULT-NOEXCITE";
    case ST_OPEN:     return "FAULT-OPEN";
    case ST_SHORT:    return "FAULT-SHORT";
    case ST_BACKFEED: return "FAULT-BACKFEED";
  }
  return "?";
}

// ===================================================================
//  Serial log
// ===================================================================
void printOr(const char *label, float v, int digits, const char *unit) {
  Serial.print(label);
  if (isnan(v)) Serial.print(F("--"));
  else { Serial.print(v, digits); Serial.print(unit); }
}

// Raw values for checking the wiring: pin millivolts (after ADC_OFFSET_MV),
// the voltages before the dividers, and the resistance before corrections.
void logRaw() {
  float ratio = senderMode == SENDER_MODE_A ? DIVIDER_RATIO : VTOP_RATIO;
  Serial.print(F("Raw: GPIO4:"));
  if (isnan(rawRefV)) Serial.print(F("--")); else { Serial.print((int)lround(rawRefV / ratio * 1000)); Serial.print(F("mV")); }
  Serial.print(F("  GPIO5:"));
  if (isnan(rawSigV)) Serial.print(F("--")); else { Serial.print((int)lround(rawSigV / ratio * 1000)); Serial.print(F("mV")); }
  printOr(senderMode == SENDER_MODE_A ? "  Feed:" : "  Vtop:", rawRefV, 2, "V");
  printOr(senderMode == SENDER_MODE_A ? "  F:" : "  Vs:", rawSigV, 2, "V");
  printOr("  Rp:", rawRp, 1, "ohm");
  Serial.print(F("  Pairs:"));
  Serial.print(rawPairs);
  if (senderMode == SENDER_MODE_B) {
    Serial.print(F("  OffHits:"));
    Serial.print(rawOffHits);
    Serial.print('/');
    Serial.print(rawOffSamples);
  }
  Serial.println();
}

void logLine() {
  printOr("Coolant: ", coolantC, 1, "C");
  printOr("  Rs:", senderOhms, 1, "ohm");
  printOr("  k:", ratioK, 3, "");
  if (senderMode == SENDER_MODE_A) printOr("  Feed:", refV, 1, "V");
  else printOr("  Vtop:", refV, 2, "V");
  printOr("  Batt:", battV, 1, "V");
  Serial.print(senderMode == SENDER_MODE_A ? F("  Mode:A") : F("  Mode:B"));
  Serial.print(F("  State:"));
  Serial.println(stateName());
}

// ===================================================================
//  Buzzer
// ===================================================================
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

// ===================================================================
//  Demo RPM / speed (no sensors yet: shown with a DEMO label)
// ===================================================================
void updateDemo() {
  float t = millis() / 1000.0;
  float drive = 0.5 + 0.5 * sin(t / 7.0);
  demoRpm = 720 + 1600 * drive * drive;
  demoSpd = demoRpm < 900 ? 0 : (demoRpm - 900) / 1500.0 * 90.0;
}

// ===================================================================
//  UI: design grid is 480x320, scaled to the panel size
// ===================================================================
uint16_t C_BG, C_CREAM, C_ORANGE, C_DIM, C_LINE, C_RED;

int16_t X(int v) { return (int32_t)v * tft.width() / 480; }
int16_t Y(int v) { return (int32_t)v * tft.height() / 320; }
void textSize(int n) {
  int sx = max(1, (int)lround(n * tft.width() / 480.0));
  int sy = max(1, (int)lround(n * tft.height() / 320.0));
  tft.setTextSize(sx, sy);
}

void text(int x, int y, int size, uint16_t color, const char *s) {
  textSize(size);
  tft.setTextColor(color);
  tft.setCursor(X(x), Y(y));
  tft.print(s);
}

// Redraw a value only when its text changed. Slot = index into shown[].
void value(int slot, int x, int y, int w, int h, int size, uint16_t color, const char *s) {
  if (strcmp(shown[slot], s) == 0) return;
  strncpy(shown[slot], s, sizeof(Txt) - 1);
  shown[slot][sizeof(Txt) - 1] = 0;
  tft.fillRect(X(x), Y(y), X(x + w) - X(x), Y(y + h) - Y(y), C_BG);
  text(x, y, size, color, s);
}

void frame(int x, int y, int w, int h, uint16_t color) {
  tft.drawRect(X(x), Y(y), X(x + w) - X(x), Y(y + h) - Y(y), color);
}

void degreeC(int x, int y, int size, uint16_t color) {
  tft.drawCircle(X(x + size * 2), Y(y + size * 2), max(1, X(size * 2) / 2 + 1), color);
  text(x + size * 4, y, size, color, "C");
}

// Segment bar: filled fraction in orange (or red), rest dim.
void bar(int x, int y, int w, int h, int segs, float frac, uint16_t color) {
  frac = constrain(frac, 0.0f, 1.0f);
  int filled = (int)lround(frac * segs);
  int segW = w / segs;
  for (int i = 0; i < segs; i++) {
    tft.fillRect(X(x + i * segW), Y(y), max(1, X(x + i * segW + segW - 2) - X(x + i * segW)), Y(y + h) - Y(y),
                 i < filled ? color : C_DIM);
  }
}

// 4-bit packed art, scaled to the panel width, one row per SPI burst.
void drawArt(const uint8_t *pix, const uint16_t *pal, int aw, int ah, int y0) {
  static uint16_t line[480];
  int dw = tft.width();
  int dh = (int32_t)ah * dw / aw;
  for (int dy = 0; dy < dh; dy++) {
    int sy = (int32_t)dy * ah / dh;
    for (int dx = 0; dx < dw && dx < 480; dx++) {
      int sx = (int32_t)dx * aw / dw;
      uint32_t i = (uint32_t)sy * aw + sx;
      uint8_t b = pix[i >> 1];
      line[dx] = pal[(i & 1) ? (b & 0x0F) : (b >> 4)];
    }
    tft.drawRGBBitmap(0, y0 + dy, line, min(dw, 480), 1);
  }
}

const char *fmtInt(Txt out, float v) {
  if (isnan(v)) strcpy(out, "--"); else snprintf(out, sizeof(Txt), "%d", (int)lround(v));
  return out;
}
const char *fmt1(Txt out, float v) {
  if (isnan(v)) strcpy(out, "--"); else snprintf(out, sizeof(Txt), "%.1f", v);
  return out;
}

// Battery tile text: the voltage, or "USB" when there is no battery divider.
const char *battText(Txt out) {
  if (!batterySense) { strcpy(out, "USB"); return out; }
  return fmt1(out, battV);
}

uint16_t waterColor() { return isAlert(state) || isFault(state) ? C_RED : C_ORANGE; }

const char *waterLabel(const char *normal) {
  switch (state) {
    case ST_OPEN:     return "SENDER OPEN";
    case ST_SHORT:    return "SENDER SHORT";
    case ST_BACKFEED: return "WIRE HAS VOLTAGE";
    case ST_NOFEED:   return senderMode == SENDER_MODE_A ? "NO GAUGE FEED" : "NO EXCITATION";
    case ST_ALARM:    return "WATER HOT!";
    case ST_WARN:     return "WATER WARM";
    default:          return normal;
  }
}

float waterFrac() { return isnan(coolantC) ? 0 : (coolantC - 40) / 80.0; }
float battFrac() { return isnan(battV) ? 0 : (battV - 11.0) / 4.0; }

void forceRedraw() {
  screenDrawn = false;
  for (auto &s : shown) s[0] = 0;
}

void drawBoot() {
  tft.fillScreen(C_BG);
  drawArt(BOOT_TITLE_PIX, BOOT_TITLE_PAL, BOOT_TITLE_W, BOOT_TITLE_H, (tft.height() - BOOT_TITLE_H * tft.width() / 480) / 2);
}

void drawCheckStatic() {
  tft.fillScreen(C_BG);
  drawArt(BOOT_TRUCK_PIX, BOOT_TRUCK_PAL, BOOT_TRUCK_W, BOOT_TRUCK_H, 0);
  frame(60, 248, 360, 22, C_CREAM);
  text(118, 282, 2, C_CREAM, "CHECKING SYSTEMS...");
  text(60, 306, 1, senderMode == SENDER_MODE_B ? C_RED : C_DIM,
       senderMode == SENDER_MODE_A ? "MODE A: PASSIVE TAPS, DASH GAUGE ON" : "MODE B: DASH GAUGE DISCONNECTED");
}

void updateCheck(unsigned long inScreen) {
  float frac = min(1.0f, inScreen / (float)CHECK_MS);
  tft.fillRect(X(63), Y(251), (X(417) - X(63)) * frac, Y(267) - Y(251), C_ORANGE);
  if (inScreen >= CHECK_MS) {
    Txt b, line;
    const char *sender = isFault(state) ? stateName() : isnan(coolantC) ? "SENDER --" : "SENDER OK";
    if (batterySense) snprintf(line, sizeof(Txt), "%s  BATT %sV", sender, fmt1(b, battV));
    else snprintf(line, sizeof(Txt), "%s  USB POWER", sender);
    value(0, 60, 282, 400, 18, 2, isFault(state) ? C_RED : C_CREAM, line);
  }
}

void drawMainStatic() {
  tft.fillScreen(C_BG);
  uint16_t fc = isAlert(state) ? C_RED : C_LINE;
  frame(4, 4, 472, 312, fc);
  tft.drawFastVLine(X(350), Y(10), Y(160) - Y(10), C_LINE);
  tft.drawFastHLine(X(10), Y(166), X(470) - X(10), C_LINE);
  tft.drawFastVLine(X(240), Y(172), Y(310) - Y(172), C_LINE);
  text(20, 14, 3, C_CREAM, "RPM");
  text(365, 14, 3, C_CREAM, "SPD");
  text(410, 104, 2, C_CREAM, "km/h");
  for (int i = 0; i <= 5; i++) {
    char d[2] = {(char)('0' + i), 0};
    text(20 + i * 62, 140, 2, C_CREAM, d);
  }
  text(400, 140, 2, C_CREAM, "x1000");
  text(300, 14, 2, C_CREAM, "DEMO");
  text(420, 22, 2, C_CREAM, "DEMO");
  text(410, 238, 3, C_CREAM, "v");
}

void updateMain() {
  Txt t, w;
  value(1, 20, 44, 260, 56, 7, C_CREAM, fmtInt(t, demoRpm));
  bar(20, 112, 320, 22, 30, demoRpm / 5000.0, C_ORANGE);
  value(2, 380, 44, 95, 56, 7, C_CREAM, fmtInt(t, demoSpd));
  value(3, 20, 180, 215, 24, 3, waterColor(), waterLabel("WATER"));
  fmtInt(w, coolantC);
  bool wNew = strcmp(shown[4], w) != 0;
  value(4, 20, 214, 140, 48, 6, waterColor(), w);
  if (wNew) degreeC(20 + 36 * strlen(w) + 4, 214, 2, waterColor());
  bar(20, 282, 200, 16, 20, waterFrac(), waterColor());
  text(255, 180, 3, C_CREAM, "BATT");
  value(5, 255, 214, 150, 48, 6, C_CREAM, battText(t));
  bar(255, 282, 200, 16, 20, battFrac(), C_ORANGE);
}

void drawGridStatic() {
  tft.fillScreen(C_BG);
  uint16_t fc = isAlert(state) ? C_RED : C_LINE;
  frame(4, 4, 472, 312, fc);
  tft.drawFastVLine(X(240), Y(10), Y(310) - Y(10), C_LINE);
  tft.drawFastHLine(X(10), Y(160), X(470) - X(10), C_LINE);
  text(20, 16, 2, C_CREAM, "RPM");
  text(255, 16, 2, C_CREAM, "SPEED");
  text(180, 16, 2, C_CREAM, "DEMO");
  text(420, 16, 2, C_CREAM, "DEMO");
  text(255, 172, 2, C_CREAM, "BATTERY");
  text(400, 80, 2, C_CREAM, "km/h");
  text(440, 248, 2, C_CREAM, "v");
}

void updateGrid() {
  Txt t, w;
  value(1, 20, 44, 200, 48, 6, C_CREAM, fmtInt(t, demoRpm));
  bar(20, 112, 200, 16, 20, demoRpm / 5000.0, C_ORANGE);
  value(2, 255, 44, 140, 48, 6, C_CREAM, fmtInt(t, demoSpd));
  bar(255, 112, 200, 16, 20, demoSpd / 120.0, C_ORANGE);
  value(3, 20, 172, 215, 18, 2, waterColor(), waterLabel("WATER TEMP"));
  fmtInt(w, coolantC);
  bool wNew = strcmp(shown[4], w) != 0;
  value(4, 20, 200, 150, 48, 6, waterColor(), w);
  if (wNew) degreeC(20 + 36 * strlen(w) + 4, 200, 2, waterColor());
  bar(20, 270, 200, 16, 20, waterFrac(), waterColor());
  value(5, 255, 200, 170, 48, 6, C_CREAM, battText(t));
  bar(255, 270, 200, 16, 20, battFrac(), C_ORANGE);
}

void drawWaterStatic() {
  tft.fillScreen(C_BG);
  uint16_t fc = isAlert(state) ? C_RED : C_LINE;
  frame(6, 6, 292, 200, fc);
  frame(306, 6, 168, 200, C_LINE);
  frame(6, 214, 230, 100, C_LINE);
  frame(244, 214, 230, 100, C_LINE);
  text(350, 112, 2, C_CREAM, "RPM");
  text(20, 226, 2, C_CREAM, "SPEED");
  text(258, 226, 2, C_CREAM, "BATT");
  text(420, 20, 2, C_CREAM, "DEMO");
  text(180, 226, 2, C_CREAM, "DEMO");
  text(150, 286, 2, C_CREAM, "km/h");
  text(400, 286, 2, C_CREAM, "v");
}

void updateWater() {
  Txt t, w;
  fmtInt(w, coolantC);
  bool wNew = strcmp(shown[4], w) != 0;
  value(4, 30, 30, 220, 96, 12, waterColor(), w);
  if (wNew) degreeC(30 + 72 * strlen(w) + 6, 30, 3, waterColor());
  value(3, 30, 140, 260, 24, 3, waterColor(), waterLabel("WATER TEMP"));
  bar(30, 176, 250, 18, 24, waterFrac(), waterColor());
  value(1, 320, 40, 150, 48, 6, C_CREAM, fmtInt(t, demoRpm));
  bar(320, 140, 140, 18, 14, demoRpm / 5000.0, C_ORANGE);
  value(2, 20, 250, 120, 48, 6, C_CREAM, fmtInt(t, demoSpd));
  value(5, 258, 250, 140, 48, 6, C_CREAM, battText(t));
}

void pollButton() {
  int b = digitalRead(BUTTON_PIN);
  unsigned long now = millis();
  if (b != lastBtn && now - lastBtnChange >= DEBOUNCE_MS) {
    lastBtnChange = now;
    lastBtn = b;
    if (b == LOW && screen >= SCR_MAIN) {
      userScreen = userScreen == SCR_MAIN ? SCR_GRID : userScreen == SCR_GRID ? SCR_WATER : SCR_MAIN;
    }
  }
}

void serviceUi() {
  unsigned long now = millis();
  Screen want = screen;
  if (screen == SCR_BOOT && now - screenSince >= BOOT_MS) want = SCR_CHECK;
  else if (screen == SCR_CHECK && now - screenSince >= CHECK_MS + RESULT_MS) want = SCR_MAIN;
  else if (screen >= SCR_MAIN) want = isAlert(state) ? SCR_WATER : userScreen;
  if (want != screen) { screen = want; screenSince = now; forceRedraw(); }

  static State drawnState = ST_WAIT;
  if (screen >= SCR_MAIN && isAlert(state) != isAlert(drawnState)) forceRedraw();  // frame colour
  drawnState = state;

  if (!screenDrawn) {
    switch (screen) {
      case SCR_BOOT:  drawBoot(); break;
      case SCR_CHECK: drawCheckStatic(); break;
      case SCR_MAIN:  drawMainStatic(); break;
      case SCR_GRID:  drawGridStatic(); break;
      case SCR_WATER: drawWaterStatic(); break;
    }
    screenDrawn = true;
    drawnScreen = screen;
    lastUi = 0;
  }
  if (lastUi != 0 && now - lastUi < UI_MS) return;
  lastUi = now;
  updateDemo();
  switch (screen) {
    case SCR_BOOT:  break;
    case SCR_CHECK: updateCheck(now - screenSince); break;
    case SCR_MAIN:  updateMain(); break;
    case SCR_GRID:  updateGrid(); break;
    case SCR_WATER: updateWater(); break;
  }
  if (state == ST_ALARM && screen >= SCR_MAIN) {
    flashOn = !flashOn;
    frame(2, 2, 476, 316, flashOn ? C_RED : C_BG);
  }
}

// ===================================================================
//  setup / loop
// ===================================================================
void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  pinMode(EXCITE_PIN, OUTPUT);
  digitalWrite(EXCITE_PIN, LOW);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  analogReadResolution(12);
  analogSetPinAttenuation(FEED_PIN, ADC_11db);
  analogSetPinAttenuation(SENDER_PIN, ADC_11db);
  analogSetPinAttenuation(BATT_PIN, ADC_11db);

  C_BG = DIYables_TFT_SPI::colorRGB(0, 0, 0);
  C_CREAM = DIYables_TFT_SPI::colorRGB(240, 200, 150);
  C_ORANGE = DIYables_TFT_SPI::colorRGB(235, 115, 40);
  C_DIM = DIYables_TFT_SPI::colorRGB(70, 40, 20);
  C_LINE = DIYables_TFT_SPI::colorRGB(200, 140, 90);
  C_RED = DIYables_TFT_SPI::colorRGB(230, 40, 30);

  tft.begin();
  tft.setRotation(1);
  tft.initBacklight(TFT_BL);
  tft.setBacklight(255);

  Serial.print(F("hj60_coolant boot ok  Mode:"));
  Serial.println(senderMode == SENDER_MODE_A ? F("A (passive taps)") : F("B (direct, DASH GAUGE DISCONNECTED)"));
  Serial.print(F("Config: Mode:"));
  Serial.print(senderMode == SENDER_MODE_A ? 'A' : 'B');
  if (senderMode == SENDER_MODE_A) { Serial.print(F("  Rg:")); Serial.print(GAUGE_R_OHMS, 1); Serial.print(F("ohm  Div:")); Serial.print(DIVIDER_RATIO, 3); }
  else {
    Serial.print(F("  Rref:")); Serial.print(REF_R_OHMS, 0); Serial.print(F("ohm  Div:")); Serial.print(VTOP_RATIO, 3);
    Serial.print(F("  Load:")); Serial.print(VS_LOAD_OHMS, 0); Serial.print(F("ohm"));
  }
  Serial.print(F("  Warn:")); Serial.print(WARN_C, 0); Serial.print(F("C  Alarm:")); Serial.print(ALARM_C, 0);
  Serial.print(F("C  AdcOffset:")); Serial.print(ADC_OFFSET_MV); Serial.print(F("mV  BatterySense:"));
  Serial.println(batterySense ? 1 : 0);
  lastValid = millis();
  windowStart = millis();
  screenSince = millis();
}

void loop() {
  unsigned long now = millis();
  if (senderMode == SENDER_MODE_A) {
    if (now - lastSample >= SAMPLE_MS) {
      lastSample = now;
      samplePairA();
    }
  } else {
    serviceExcitationB();
  }
  if (batterySense && now - lastBatt >= 50) {
    lastBatt = now;
    battSum += pinVolts(BATT_PIN, DIVIDER_RATIO);
    battCount++;
  }
  if (now - windowStart >= WINDOW_MS) {
    windowStart = now;
    evaluateWindow();
    logRaw();
    logLine();
  }
  driveBuzzer();
  pollButton();
  serviceUi();
}
