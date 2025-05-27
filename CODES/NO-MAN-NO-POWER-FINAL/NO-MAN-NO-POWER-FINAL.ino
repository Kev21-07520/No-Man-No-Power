#include <Wire.h>
#include <RTClib.h>
#include <LiquidCrystal_I2C.h>

RTC_DS3231 rtc;
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Pins
#define PIR_PIN 14
#define RELAY1_PIN 27  // Channel 1 - DC Motor
#define RELAY2_PIN 26  // Channel 2 - LEDs
#define SWITCH_PIN 34

unsigned long lastMotionTime = 0;
unsigned long motionTimeout = 30000;  // 30 seconds for testing
bool systemOn = false;
bool systemSleeping = false;

void setup() {
  pinMode(PIR_PIN, INPUT);
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  pinMode(SWITCH_PIN, INPUT);

  digitalWrite(RELAY1_PIN, LOW);  // OFF initially
  digitalWrite(RELAY2_PIN, LOW);

  Wire.begin();
  rtc.begin();
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("System Ready");
}

void loop() {
  bool switchState = digitalRead(SWITCH_PIN) == HIGH;

  if (!switchState) {
    // Master switch OFF
    if (systemOn || systemSleeping) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("SWITCH OFF");
      lcd.setCursor(0, 1);
      lcd.print("System OFF");
      digitalWrite(RELAY1_PIN, LOW);
      digitalWrite(RELAY2_PIN, LOW);
      systemOn = false;
      systemSleeping = false;
    }
    delay(500);
    return;
  }

  // Master switch is ON and system is not yet running
  if (!systemOn && !systemSleeping) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("SWITCH ON");
    lcd.setCursor(0, 1);
    lcd.print("System ON");
    digitalWrite(RELAY1_PIN, HIGH);
    digitalWrite(RELAY2_PIN, HIGH);
    systemOn = true;
    lastMotionTime = millis(); // Start motion timer
    delay(2000);
  }

  // Motion detected
  if (systemOn && digitalRead(PIR_PIN) == HIGH) {
    lastMotionTime = millis();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Motion Detected");
    lcd.setCursor(0, 1);
    lcd.print("Appliance ON");
    digitalWrite(RELAY1_PIN, HIGH);
    digitalWrite(RELAY2_PIN, HIGH);
  }

  // Inactivity timeout
  if (systemOn && (millis() - lastMotionTime > motionTimeout)) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("No Motion Found");
    lcd.setCursor(0, 1);
    lcd.print("Turning Off...");
    digitalWrite(RELAY1_PIN, LOW);
    digitalWrite(RELAY2_PIN, LOW);
    systemOn = false;
    systemSleeping = true;
    delay(2000);
  }

  // Reactivate after sleeping if motion is detected again
  if (systemSleeping && digitalRead(PIR_PIN) == HIGH) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Motion Detected");
    lcd.setCursor(0, 1);
    lcd.print("System Re-On");
    digitalWrite(RELAY1_PIN, HIGH);
    digitalWrite(RELAY2_PIN, HIGH);
    systemSleeping = false;
    systemOn = true;
    lastMotionTime = millis();
    delay(2000);
  }

  delay(200);
}
