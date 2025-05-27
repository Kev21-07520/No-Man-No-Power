#include <Wire.h>
#include <RTClib.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>

RTC_DS3231 rtc;
LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo servo;

#define PIR_PIN 14
#define LED_PIN 26
#define RELAY_PIN 27
#define SERVO_PIN 13
#define SWITCH_PIN 34

unsigned long lastMotionTime = 0;
unsigned long motionTimeout = 30000;  // 30 seconds
bool motionDetected = false;
bool systemOn = false;
bool servoMoved = false;
bool systemSleeping = false;

void setup() {
  pinMode(PIR_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(SWITCH_PIN, INPUT);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(RELAY_PIN, LOW);

  servo.attach(SERVO_PIN);
  servo.write(90); // Center position

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
    if (systemOn) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("SWITCH OFF");
      lcd.setCursor(0, 1);
      lcd.print("System OFF");
      digitalWrite(LED_PIN, LOW);
      digitalWrite(RELAY_PIN, LOW);
      servo.detach();
      systemOn = false;
      servoMoved = false;
      systemSleeping = false;
    }
    delay(500);
    return;
  }

  // Master switch ON but system is sleeping
  if (systemSleeping) {
    // Wait for new motion to wake the system
    if (digitalRead(PIR_PIN) == HIGH) {
      systemSleeping = false;
      systemOn = false; // Allow reactivation
    } else {
      delay(200);
      return;
    }
  }

  // Master switch ON and system not yet activated
  if (!systemOn) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("SWITCH ON");
    lcd.setCursor(0, 1);
    lcd.print("System ON");
    systemOn = true;
    lastMotionTime = millis(); // Start timeout
    motionDetected = false;
    servoMoved = false;
    servo.attach(SERVO_PIN);
    servo.write(90);
    digitalWrite(LED_PIN, HIGH);
    digitalWrite(RELAY_PIN, HIGH);
    delay(2000);
  }

  // Motion Detection
  if (digitalRead(PIR_PIN) == HIGH) {
    lastMotionTime = millis();
    if (!servoMoved) {
      motionDetected = true;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Motion Detected");

      // Sweep servo
      for (int pos = 90; pos <= 135; pos++) {
        servo.write(pos);
        delay(10);
      }
      for (int pos = 135; pos >= 45; pos--) {
        servo.write(pos);
        delay(10);
      }
      for (int pos = 45; pos <= 90; pos++) {
        servo.write(pos);
        delay(10);
      }

      lcd.setCursor(0, 1);
      lcd.print("Appliance ON");
      servoMoved = true;
    }
  }

  // Inactivity timeout
  if (millis() - lastMotionTime > motionTimeout) {
    digitalWrite(LED_PIN, LOW);
    digitalWrite(RELAY_PIN, LOW);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("No Motion Found");
    lcd.setCursor(0, 1);
    lcd.print("Turning Off...");
    servo.detach();
    systemOn = false;
    servoMoved = false;
    systemSleeping = true;  // Prevent auto reactivation
    delay(2000);
  } else {
    digitalWrite(LED_PIN, HIGH);
    digitalWrite(RELAY_PIN, HIGH);
  }

  delay(200);
}
