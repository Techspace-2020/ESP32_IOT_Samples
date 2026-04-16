/************ LIBRARIES ************/
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

/************ CONFIG ************/
#define BLYNK_AUTH "YourAuthToken"
#define WIFI_SSID  "Rakesh Rocky"
#define WIFI_PASS  "nu1rs59j4k"

/************ PIN SETUP ************/
#define LED_GREEN  2
#define LED_RED    4
#define BUZZER     5

LiquidCrystal_I2C lcd(0x27, 16, 2);

/************ GLOBAL FLAGS ************/
bool wifiConnected = false;
bool blynkConnected = false;

/************ SETUP ************/
void setup() {
  Serial.begin(115200);

  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  lcd.init();
  lcd.backlight();

  connectWiFi();
  connectBlynk();
}

/************ LOOP ************/
void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    digitalWrite(LED_GREEN, HIGH);   // WiFi OK
    digitalWrite(LED_RED, LOW);
  } else {
    wifiConnected = false;
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_RED, HIGH);
    reconnectWiFi();                 // Retry WiFi
  }

  if (wifiConnected) {
    if (Blynk.connected()) {
      blynkConnected = true;
      Blynk.run();
    } else {
      blynkConnected = false;
      reconnectBlynk();              // Retry Blynk
    }
  }

  statusDisplay();                   // LCD + buzzer indication
}

/************ FUNCTIONS ************/
void connectWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  lcd.print("Connecting WiFi");
  delay(3000);
}

void reconnectWiFi() {
  WiFi.disconnect();
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  delay(2000);
}

void connectBlynk() {
  Blynk.config(BLYNK_AUTH);
  Blynk.connect();
}

void reconnectBlynk() {
  Blynk.connect();
}

void statusDisplay() {
  lcd.clear();

  if (wifiConnected && blynkConnected) {
    lcd.print("System Online");
    digitalWrite(BUZZER, LOW);
  } 
  else {
    lcd.print("Check System");
    digitalWrite(BUZZER, HIGH);   // Alert
    delay(200);
    digitalWrite(BUZZER, LOW);
  }
}