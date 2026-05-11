#define BLYNK_TEMPLATE_ID "TMPL3G_6WN0JO"
#define BLYNK_TEMPLATE_NAME "Construction Site Safety Monitoring Lanchan"
#define BLYNK_AUTH_TOKEN "C-hBn6Mhvqg11OeqUdVGDiItUx9fHVA9"

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <DHT.h>
#include "MQ135.h"

// WiFi Credentials
char ssid[] = "JioFiber4G";
char pass[] = "Animal1234";

// Pin Definitions
#define RED_LED_1 4
#define RED_LED_2 5

#define BLUE_LED_1 2
#define BLUE_LED_2 16

#define GREEN_LED_1 0
#define GREEN_LED_2 17
#define Buzzer 19

#define DHTPIN 15
#define DHTTYPE DHT11

#define MQ135_PIN 34

// Clean air baseline
#define CLEAN_AIR_RATIO 3.6
#define BASE_CO2_PPM 400.0

DHT dht(DHTPIN, DHTTYPE);

#define RL_VALUE 10.0
#define ADC_MAX 4095.0
#define VREF 3.3

// Calibration
float R0 = 0;
bool calibrated = false;
int calibrationCount = 0;

// Filtering
float filteredRatio = 0;
float alpha = 0.1;

// Connection Flags
bool isWiFiConnected = false;
bool isBlynkConnected = false;
bool alert_flag = false;
bool previous_alert = false;

BlynkTimer timer;

// ---------------- CONNECTION CHECK ----------------
void checkConnections() {
  if (WiFi.status() == WL_CONNECTED) {
    isWiFiConnected = true;
    digitalWrite(BLUE_LED_1, HIGH);
    digitalWrite(RED_LED_1, LOW);
  } else {
    isWiFiConnected = false;
    digitalWrite(BLUE_LED_1, LOW);
    digitalWrite(RED_LED_1, HIGH);
    Serial.println("WiFi Lost. Attempting to reconnect...");
    WiFi.begin(ssid, pass);
  }

  isBlynkConnected = Blynk.connected();

  if (isBlynkConnected) {
    digitalWrite(GREEN_LED_1, HIGH);
    Serial.println("System Online: WiFi & Blynk OK");
  } else {
    digitalWrite(GREEN_LED_1, LOW);
    if (isWiFiConnected) {
      Blynk.connect();
    }
  }

  if (isWiFiConnected && isBlynkConnected) {
    Blynk.virtualWrite(V1, "SYSTEM ACTIVE");
  } else {
    Blynk.virtualWrite(V1, "SYSTEM OFFLINE");
  }
}

// ---------------- SETUP ----------------
void setup() {
  Serial.begin(115200);

  pinMode(RED_LED_1, OUTPUT);
  pinMode(RED_LED_2, OUTPUT);
  pinMode(BLUE_LED_1, OUTPUT);
  pinMode(BLUE_LED_2, OUTPUT);
  pinMode(GREEN_LED_1, OUTPUT);
  pinMode(GREEN_LED_2, OUTPUT);
  pinMode(Buzzer, OUTPUT);

  dht.begin();

  analogReadResolution(12);
  analogSetPinAttenuation(MQ135_PIN, ADC_11db);

  Serial.println("MQ135 Multi-Gas Monitor (ESP32)");

  tone(Buzzer, 2000, 200);

  WiFi.begin(ssid, pass);
  Blynk.config(BLYNK_AUTH_TOKEN);

  timer.setInterval(5000L, checkConnections);
  timer.setInterval(2000L, readSensors);
}

// ---------------- LOOP ----------------
void loop() {
  timer.run();

  if (WiFi.status() == WL_CONNECTED) {
    Blynk.run();
  }

  // 🔔 Buzzer logic (state-based)
  if (alert_flag && !previous_alert) {
    tone(Buzzer, 2000, 500);  // Beep once when alert starts
    digitalWrite(RED_LED_2, HIGH);
    digitalWrite(GREEN_LED_2, LOW);
  }

  if (!alert_flag) {
    noTone(Buzzer);
    digitalWrite(RED_LED_2, LOW);
    digitalWrite(GREEN_LED_2, HIGH);
  }

  previous_alert = alert_flag;
}

// ---------------- SENSOR READING ----------------
void readSensors() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (!isnan(h) && !isnan(t)) {
    Blynk.virtualWrite(V2, t);
    Blynk.virtualWrite(V3, h);
  } else {
    Serial.println("Failed to read from DHT sensor!");
  }

  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.println(" °C");

  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.println(" %");

  float Rs = readRs();

  if (Rs <= 0) {
    Serial.println("⚠️ Sensor error");
    return;
  }

  // ----------- CALIBRATION -----------
  if (!calibrated) {
    R0 += Rs;
    calibrationCount++;

    Serial.println("Calibrating MQ135...");

    if (calibrationCount >= 30) {
      R0 = (R0 / 30) / CLEAN_AIR_RATIO;
      calibrated = true;
      Serial.println("Calibration Done!");
    }
    return;
  }

  float ratio = Rs / R0;

  // ----------- FILTER -----------
  if (filteredRatio == 0)
    filteredRatio = ratio;

  filteredRatio = alpha * ratio + (1 - alpha) * filteredRatio;

  // ----------- GAS CALCULATIONS -----------
  float co2 = getCO2(filteredRatio);
  float nh3 = getNH3(filteredRatio);
  float alcohol = getAlcohol(filteredRatio);
  float benzene = getBenzene(filteredRatio);
  float smoke = getSmoke(filteredRatio);

  // ----------- OUTPUT -----------
  Serial.print("Ratio: "); Serial.print(filteredRatio, 2);

  Serial.print(" | CO2: "); Serial.print(co2, 0); Serial.print(" ppm");
  Serial.print(" | NH3: "); Serial.print(nh3, 0); Serial.print(" ppm");
  Serial.print(" | Alcohol: "); Serial.print(alcohol, 0); Serial.print(" ppm");
  Serial.print(" | Benzene: "); Serial.print(benzene, 0); Serial.print(" ppm");
  Serial.print(" | Smoke: "); Serial.print(smoke, 0); Serial.print(" ppm");

  Serial.print(" | Air: ");
  Blynk.virtualWrite(V4, co2);
  Blynk.virtualWrite(V5, nh3);
  Blynk.virtualWrite(V6, alcohol);

  if (filteredRatio > 3.5) Serial.println("Clean 🌿");
  else if (filteredRatio > 2.5) Serial.println("Moderate 😐");
  else if (filteredRatio > 1.8) Serial.println("Poor ⚠️");
  else Serial.println("Very Poor ☠️");

  // ----------- UPDATED ALERT LOGIC -----------
  if (filteredRatio < 2.5) {
    alert_flag = true;   // Trigger for Poor + Very Poor
  } else {
    alert_flag = false;
  }
}

// ---------------- MQ135 CORE ----------------
float readRs() {
  int adc = analogRead(MQ135_PIN);
  if (adc <= 0) return -1;

  float voltage = adc * (VREF / ADC_MAX);
  float Rs = ((VREF - voltage) / voltage) * RL_VALUE;
  return Rs;
}

// ---------------- GAS MODELS ----------------
float getCO2(float ratio) {
  return BASE_CO2_PPM * pow(CLEAN_AIR_RATIO / ratio, 2.0);
}

float getNH3(float ratio) {
  return 102.2 * pow(ratio, -2.473);
}

float getAlcohol(float ratio) {
  return 77.255 * pow(ratio, -3.18);
}

float getBenzene(float ratio) {
  return 44.947 * pow(ratio, -3.445);
}

float getSmoke(float ratio) {
  return 150.0 * pow(ratio, -2.7);
}
