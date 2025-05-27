#define BLYNK_TEMPLATE_ID "TMPL6lXBA8oxd"
#define BLYNK_TEMPLATE_NAME "No Man No Power"
#define BLYNK_AUTH_TOKEN "Zke3cElnFO1mW3J9-4fw5XHqCjn9OSJQ"

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <RTClib.h>
#include <LiquidCrystal_I2C.h>

RTC_DS3231 rtc;
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Pin definitions
#define PIR_PIN 14
#define RELAY1_PIN 27  // DC Motor
#define RELAY2_PIN 26  // LEDs
#define SWITCH_PIN 34  // Physical master switch
#define BLYNK_ONOFF_VPIN V0

// WiFi credentials
char ssid[] = "Kevin Arellano";
char pass[] = "1234567890";

// System states
unsigned long lastMotionTime = 0;
unsigned long motionTimeout = 30000;  // 30 seconds
bool systemOn = false;
bool systemSleeping = false;
bool blynkEnabled = true;

BLYNK_WRITE(BLYNK_ONOFF_VPIN) {
  blynkEnabled = param.asInt(); // 1 = ON, 0 = OFF
}

void setup() {
  // Serial and pin setup
  Serial.begin(115200);
  pinMode(PIR_PIN, INPUT);
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  pinMode(SWITCH_PIN, INPUT);

  digitalWrite(RELAY1_PIN, LOW);
  digitalWrite(RELAY2_PIN, LOW);

  // Start modules
  Wire.begin();
  rtc.begin();
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi");

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("System Ready");
}

void loop() {
  Blynk.run();

  bool switchState = digitalRead(SWITCH_PIN) == HIGH;

  // If physical switch is OFF or Blynk turned OFF, system must stop
  if (!switchState || !blynkEnabled) {
    if (systemOn || systemSleeping) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(switchState ? "Blynk: OFF" : "SWITCH: OFF");
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

  // Switch is ON and Blynk is enabled
  if (!systemOn && !systemSleeping) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("SWITCH ON");
    lcd.setCursor(0, 1);
    lcd.print("System ON");
    digitalWrite(RELAY1_PIN, HIGH);
    digitalWrite(RELAY2_PIN, HIGH);
    systemOn = true;
    lastMotionTime = millis();
    delay(1000);
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

  // Reactivate on motion if sleeping
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
