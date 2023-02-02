#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_MCP4725.h>
#include <Encoder.h>
#include <Wire.h>

#include "pinDef.h"

#define DAC_RESOLUTION  4095    // 12-bits DAC resolution
#define REF_VOLTAGE     5.0

Adafruit_SSD1306 display = Adafruit_SSD1306(128, 64, &Wire);
Adafruit_MCP4725 dac;
Encoder encoder(ENCODER_A, ENCODER_B);
int16_t dacVal = DAC_RESOLUTION / 2;     // write half the reference voltage on start
float voltage = REF_VOLTAGE / 2;
bool encoderSWFlag = false;
char msgBuffer[64];

void encoderPressed() {
  static unsigned long prevPressTime, pressTime = 0;
  pressTime = millis();
  if (pressTime - prevPressTime > DEBOUNCE_INTERVAL_MS) {
    encoderSWFlag = true;
    prevPressTime = pressTime;
  }
}


void print2Display(String msg) {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print(msg);
  display.display();
}

void setup() {
  // Serial.begin(9600);
  dac.begin(0x61); // 0x60 to 0x63
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(ENCODER_SW, INPUT_PULLUP);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3c)) {
    Serial.println("Failed to initialize OLED!");
    while (1);
  }
  display.clearDisplay();
  display.display();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  sprintf(msgBuffer, "Current\nVoltage: \n%.3f V", voltage);
  print2Display(msgBuffer);

  attachInterrupt(ENCODER_SW, encoderPressed, FALLING);
}

void loop() {
  if (encoderSWFlag) {
    encoderSWFlag = false;
    uint32_t encoderPos = encoder.read();
    unsigned long rotateStamp, lastRotateStamp = millis();
    sprintf(msgBuffer, "Set new\nVoltage:\n%.3f V", voltage);
    print2Display(msgBuffer);

    while (true) {
      if (encoderSWFlag) { encoderSWFlag = false; break; }
      uint32_t newPos = encoder.read();
      int16_t changes;
      static uint8_t fastRotateCount = 0;

      if (newPos != encoderPos) {
        rotateStamp = millis();

        if (rotateStamp - lastRotateStamp <= FAST_ROTATE_INTERVAL_MS) { fastRotateCount++; }
        else { fastRotateCount = 0; }
        if (fastRotateCount > 2) { changes = (newPos > encoderPos) ? FAST_ROTATE_AMOUNT : -FAST_ROTATE_AMOUNT; }
        else { changes = (newPos > encoderPos) ? ROTATE_AMOUNT : -ROTATE_AMOUNT; }
        lastRotateStamp = rotateStamp;

        dacVal += changes;
        dacVal = max(0, dacVal);
        dacVal = min(DAC_RESOLUTION, dacVal);
        voltage = dacVal * REF_VOLTAGE / DAC_RESOLUTION;

        encoderPos = newPos;
        sprintf(msgBuffer, "Set new\nVoltage:\n%.3f V", voltage);
        print2Display(msgBuffer);
        dac.setVoltage(dacVal, false);
      }
      delay(1);
    }

    sprintf(msgBuffer, "Current\nVoltage:\n%.3f V", voltage);
    print2Display(msgBuffer);
  }
}