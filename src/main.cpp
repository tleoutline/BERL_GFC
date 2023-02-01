#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_ADS1X15.h>
#include <Adafruit_MCP4725.h>
#include <Encoder.h>
#include <Wire.h>

#include "pinDef.h"

#define DAC_RESOLUTION 4095    // 12-bits DAC resolution

Adafruit_SSD1306 display = Adafruit_SSD1306(128, 64, &Wire);
Adafruit_MCP4725 dac;
Encoder encoder(ENCODER_A, ENCODER_B);
uint16_t dacVal = DAC_RESOLUTION / 2;     // write half the reference voltage on start
const float refVoltage = 5.0;
float voltage = refVoltage / 2;
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

void encoder2Val(long encoderChanges) {
  long changes = 0;
  if (encoderChanges > 0) {
    changes = (encoderChanges > FAST_ROTATE_COUNT ? FAST_ROTATE_AMOUNT : 10) * encoderChanges;
    dacVal = min(DAC_RESOLUTION, dacVal + changes);
  }
  else if (encoderChanges < 0) {
    changes = (-encoderChanges > FAST_ROTATE_COUNT ? -FAST_ROTATE_AMOUNT : -10) * -encoderChanges;
    dacVal = max(0L, dacVal + changes);
  }
  voltage = refVoltage * dacVal / DAC_RESOLUTION;
  return;
}

void print2Display(String msg) {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print(msg);
  display.display();
}

void setup() {
  dac.begin(0x60); // 0x60 to 0x63
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
  sprintf(msgBuffer, "Current Voltage: \n%.2f V", voltage);
  print2Display(msgBuffer);

  attachInterrupt(ENCODER_SW, encoderPressed, FALLING);
}

void loop() {
  static long encoderPos = 0;

  if (encoderSWFlag) {
    encoderSWFlag = false;
    sprintf(msgBuffer, "Set new Voltage:\n%.2f V", voltage);
    print2Display(msgBuffer);

    while (true) {
      if (encoderSWFlag) { encoderSWFlag = false; break; }
      long newPos = encoder.read();
      if (newPos != encoderPos) {
        encoder2Val(newPos - encoderPos);
        encoderPos = newPos;
        sprintf(msgBuffer, "Set new Voltage:\n%.2f V", voltage);
        print2Display(msgBuffer);
        dac.setVoltage(dacVal, false);
      }
      delay(100);
    }

    sprintf(msgBuffer, "Current Voltage:\n%.2f V", voltage);
    print2Display(msgBuffer);
  }
}