#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Encoder.h>
#include <Wire.h>

#include "pinDef.h"

Adafruit_SSD1306 display = Adafruit_SSD1306(128, 64, &Wire);
Encoder encoder(ENCODER_A, ENCODER_B);
uint16_t dacVal = 512;
float voltage = dacVal * 5 / 1023;
bool encoderSWFlag = false;

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
    changes = (encoderChanges > FAST_ROTATE_COUNT ? 5 : 1) * encoderChanges;

    dacVal = min(1023L, dacVal + changes);
  }
  else if (encoderChanges < 0) {
    changes = (-encoderChanges > FAST_ROTATE_COUNT ? -5 : -1) * -encoderChanges;
    dacVal = max(0L, dacVal + changes);
  }
  voltage = (float)dacVal * 5UL / 1023UL;
  return;
}

void setup() {
  Serial.begin(9600);
  pinMode(DAC_PIN, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(ENCODER_SW, INPUT_PULLUP);
  analogWriteResolution(10);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3c)) {
    Serial.println("Failed to initialize OLED!");
    while (1);
  }
  display.clearDisplay();
  display.display();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Current\ncontrol\nvoltage:");
  display.print(voltage, 3);
  display.println("V");
  display.display();


  attachInterrupt(ENCODER_SW, encoderPressed, FALLING);
}

void loop() {
  static long encoderPos = 0;

  if (encoderSWFlag) {
    encoderSWFlag = false;
    Serial.println("Change control voltage to:");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Set new\ncontrol\nvoltage:");
    display.print(voltage, 3);
    display.println("V");
    display.display();

    while (true) {
      if (encoderSWFlag) { encoderSWFlag = false; break; }
      long newPos = encoder.read();
      if (newPos != encoderPos) {
        encoder2Val(newPos - encoderPos);
        encoderPos = newPos;
        display.clearDisplay();
        display.setCursor(0, 0);
        display.println("Set\ncontrol\nvoltage:");
        display.print(voltage, 3);
        display.println("V");
        display.display();

        Serial.print("Value: ");
        Serial.println(dacVal);
        Serial.print(voltage, 3);
        Serial.println("V");
      }
      delay(100);
    }

    analogWrite(DAC_PIN, dacVal);
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Current\ncontrol\nvoltage:");
    display.print(voltage, 3);
    display.println("V");
    display.display();

    Serial.println("Current control voltage:");
    Serial.print(voltage, 3);
    Serial.println("V");
    Serial.println("Current value:");
    Serial.println(dacVal);
  }
}