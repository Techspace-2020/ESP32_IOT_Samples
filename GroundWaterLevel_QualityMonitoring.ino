#define BLYNK_TEMPLATE_ID "TMPL31moIx8g3"
#define BLYNK_TEMPLATE_NAME "Ground Water Level And Quality Monitoring System"
#define BLYNK_AUTH_TOKEN "p5fWW1Nj50Ap1iaPTeVu8gzs3wID_0mU"

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

// WiFi Credentials
char ssid[] = "Rakesh Rocky";
char pass[] = "nu1rs59j4k";

// Pin Definitions
#define RED_LED_1 4
#define RED_LED_2 5

#define BLUE_LED_1 2
#define BLUE_LED_2 16

#define GREEN_LED_1 0
#define GREEN_LED_2 17
#define Buzzer 19

// --- NEW: Water Level Pins ---
#define PIN_LOW 14
#define PIN_MID 27
#define PIN_FULL 26

#define TDS_PIN A0
#define VREF 3.3
#define ADC_RES 4095

float averageVoltage = 0;
float tdsValue = 0;
float temperature = 32.0;
String waterLevelStatus = "Empty";

// Connection Flags
bool isWiFiConnected = false;
bool isBlynkConnected = false;
bool tds_flag = false;
BlynkTimer timer;

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

  // Update Blynk Flag
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
  if(WiFi.status()==WL_CONNECTED && isBlynkConnected){
    Blynk.virtualWrite(V1, "SYSTEM ACTIVE");
  }else {
    Blynk.virtualWrite(V1, "SYSTEM OFFLINE");
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(RED_LED_1, OUTPUT);
  pinMode(RED_LED_2, OUTPUT);
  pinMode(BLUE_LED_1, OUTPUT);
  pinMode(BLUE_LED_2, OUTPUT);
  pinMode(GREEN_LED_1, OUTPUT);
  pinMode(GREEN_LED_2, OUTPUT);
  pinMode(Buzzer, OUTPUT);

  // --- NEW: Initialize Level Pins ---
  pinMode(PIN_LOW, INPUT_PULLUP);
  pinMode(PIN_MID, INPUT_PULLUP);
  pinMode(PIN_FULL, INPUT_PULLUP);

  tone(Buzzer, 2000, 200);
  WiFi.begin(ssid, pass);
  Blynk.config(BLYNK_AUTH_TOKEN);

  analogReadResolution(12);  // Ensure 12-bit resolution
  pinMode(TDS_PIN, INPUT);

  timer.setInterval(5000L, checkConnections);
  timer.setInterval(2000L, readTDS);
  timer.setInterval(2000L, readWaterLevel);
}

void loop() {
  timer.run();
  if (WiFi.status() == WL_CONNECTED) {
    Blynk.run();
  }
  if (tds_flag) {
    tone(Buzzer, 2000, 200);
    digitalWrite(RED_LED_2, HIGH);
    digitalWrite(GREEN_LED_2, LOW);
  } else {
    noTone(Buzzer);
    digitalWrite(RED_LED_2, LOW);
    digitalWrite(GREEN_LED_2, HIGH);
  }
}

void readTDS() {
  long sum = 0;

  for (int i = 0; i < 30; i++) {
    sum += analogRead(TDS_PIN);
    delay(10);
  }

  float rawADC = sum / 30.0;
  averageVoltage = (rawADC * VREF) / ADC_RES;

  float compensationCoefficient = 1.0 + 0.02 * (temperature - 25.0);
  float compensationVoltage = averageVoltage / compensationCoefficient;

  // Convert voltage to TDS value in ppm
  // Standard formula: TDS = (133.42 * V^3 - 255.86 * V^2 + 857.39 * V) * 0.5
  tdsValue = (133.42 * pow(compensationVoltage, 3) - 255.86 * pow(compensationVoltage, 2) + 857.39 * compensationVoltage) * 0.5;  //0.5

  // Serial.print("\r\nVoltage: ");
  // Serial.print(averageVoltage);
  Serial.print("\r\n");
  Serial.print("TDS Value: ");
  Serial.print(tdsValue);
  Serial.println(" ppm");
  if (tdsValue > 200) {
    tds_flag = true;
  } else {
    tds_flag = false;
  }
  // Send to Blynk (Virtual Pin V1)
  Blynk.virtualWrite(V3, tdsValue);
}

// --- NEW: Function to Read Water Level ---
void readWaterLevel() {
  // We use LOW because INPUT_PULLUP keeps pins HIGH until water (GND) touches them
  int lowLevel = digitalRead(PIN_LOW);
  int midLevel = digitalRead(PIN_MID);
  int fullLevel = digitalRead(PIN_FULL);
  int levelPercent =0;
  if (fullLevel == LOW) {
    waterLevelStatus = "Full (100%)";
    tone(Buzzer, 2000, 200);
    levelPercent = 100;
  } else if (midLevel == LOW) {
    waterLevelStatus = "Medium (50%)";
    levelPercent = 50;
  } else if (lowLevel == LOW) {
    waterLevelStatus = "Low (25%)";
    levelPercent = 25;
  } else {
    waterLevelStatus = "Empty (0%)";
    levelPercent = 0;
  }

  Serial.print("Water Level: ");
  Serial.println(waterLevelStatus);
  // if (!lowLevel) {
  //   // Tank empty → start motor
  //   tone(Buzzer, 2000); // buzzer ON
  //   Serial.println("Motor ON (Tank Empty)");
  // }
  // if (fullLevel) {
  //      noTone(Buzzer); // buzzer ON
  //      Serial.println("Motor OFF (Tank Full)");
  // }

  if (isBlynkConnected) {
    Blynk.virtualWrite(V4, waterLevelStatus);
    Blynk.virtualWrite(V5, levelPercent);
  }
}
