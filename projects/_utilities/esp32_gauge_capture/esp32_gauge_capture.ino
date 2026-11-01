// =====================================================================
//  esp32_gauge_capture -- record the HJ60 gauge feed + sender waveform
// ---------------------------------------------------------------------
//  Board : ESP32 DevKit V1
//  Pins  : GPIO34 = gauge feed (terminal E), GPIO35 = sender (terminal F),
//          both through the 120k/10k dividers in
//          projects/hj60_coolant/docs/gauge-tap-report.md section 4.1.
//
//  Prints CSV at 921600 baud, 1 kHz, for 30 s after reset:
//    t_ms,feed_mV,sender_mV        (ADC millivolts, before x13)
//  Then prints "done". Press EN (reset) to capture again.
//
//  arduino-cli monitor -p /dev/cu.usbserial-XXXX -c baudrate=921600 > capture.csv
// =====================================================================

#define FEED_PIN   34
#define SENDER_PIN 35
const unsigned long CAPTURE_MS = 30000;
const unsigned long PERIOD_US = 1000;

unsigned long startMs = 0;
unsigned long nextUs = 0;
bool done = false;

void setup() {
  Serial.begin(921600);
  analogReadResolution(12);
  analogSetPinAttenuation(FEED_PIN, ADC_11db);
  analogSetPinAttenuation(SENDER_PIN, ADC_11db);
  delay(500);
  Serial.println("t_ms,feed_mV,sender_mV");
  startMs = millis();
  nextUs = micros();
}

void loop() {
  if (done) return;
  if ((long)(micros() - nextUs) < 0) return;
  nextUs += PERIOD_US;
  unsigned long t = millis() - startMs;
  if (t >= CAPTURE_MS) { Serial.println("done"); done = true; return; }
  uint32_t f = analogReadMilliVolts(FEED_PIN);
  uint32_t s = analogReadMilliVolts(SENDER_PIN);
  Serial.print(t); Serial.print(','); Serial.print(f); Serial.print(','); Serial.println(s);
}
