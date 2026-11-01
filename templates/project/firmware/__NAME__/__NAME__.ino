// =====================================================================
//  __NAME__  --  __PURPOSE__
// ---------------------------------------------------------------------
//  Board : __BOARD__
//  Pins  : STATUS_LED = LED_BUILTIN   (add every pin here as you wire it)
//
//  Serial log @ __BAUD__ baud:
//    __NAME__ boot ok
//    heartbeat <seconds>
// =====================================================================

#ifndef LED_BUILTIN
#define LED_BUILTIN 2   // ESP32 DevKit boards don't define it
#endif
#define STATUS_LED LED_BUILTIN
const unsigned long HEARTBEAT_MS = 1000;

unsigned long lastBeat = 0;
bool ledOn = false;

void setup() {
  Serial.begin(__BAUD__);
  pinMode(STATUS_LED, OUTPUT);
  Serial.println(F("__NAME__ boot ok"));
}

void loop() {
  if (millis() - lastBeat >= HEARTBEAT_MS) {
    lastBeat = millis();
    ledOn = !ledOn;
    digitalWrite(STATUS_LED, ledOn ? HIGH : LOW);
    Serial.print(F("heartbeat "));
    Serial.println(millis() / 1000);
  }
}
