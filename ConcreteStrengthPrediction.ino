#define BLYNK_TEMPLATE_ID "TMPL3mM27GfBn"
#define BLYNK_TEMPLATE_NAME "Concrete Strength Prediction Ramesh"
#define BLYNK_AUTH_TOKEN "VWFOIwfozYv3ioqd40N3-iXQGQSRUY54"

#include <Wire.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

// WiFi Credentials
char ssid[] = "Rakesh Rocky";
char pass[] = "nu1rs59j4k";

// Ultrasonic - Updated Pin naming to match logic
#define TRIG_PIN 12
#define ECHO_PIN 14
#define SOUND_SPEED 0.034

#define RED_LED_1 4
#define RED_LED_2 5
#define BLUE_LED_1 2
#define BLUE_LED_2 16
#define GREEN_LED_1 0
#define GREEN_LED_2 17
#define Buzzer 19


float crackThreshold = 3.0; 
float baseDistance = 0;
bool crackDetected = false;

// Connection Flags & Timer
bool isWiFiConnected = false;
bool isBlynkConnected = false;
BlynkTimer timer;

void setup() {
  Serial.begin(115200);

  // LED & Buzzer Setup
  pinMode(RED_LED_1, OUTPUT);
  pinMode(RED_LED_2, OUTPUT);
  pinMode(BLUE_LED_1, OUTPUT);
  pinMode(GREEN_LED_1, OUTPUT);
  pinMode(GREEN_LED_2, OUTPUT);
  pinMode(Buzzer, OUTPUT);
  tone(Buzzer, 2000, 200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT); 

  delay(2000);

  // --- NEW: Initialize Base Distance ---
  baseDistance = getUltrasonicDistance();
  // If first reading fails, set a default or retry
  if(baseDistance <= 0) baseDistance = 20.0; 
  Serial.print("Base Distance Set: ");
  Serial.println(baseDistance);

  prevTime = millis();

  WiFi.begin(ssid, pass);
  Blynk.config(BLYNK_AUTH_TOKEN);
  
  timer.setInterval(5000L, checkConnections);
  timer.setInterval(1000L, sendDataToBlynk);
}

void checkConnections() {
  if (WiFi.status() == WL_CONNECTED) {
    isWiFiConnected = true;
    digitalWrite(BLUE_LED_1, HIGH); 
    digitalWrite(RED_LED_1, LOW);
  } else {
    isWiFiConnected = false;
    digitalWrite(BLUE_LED_1, LOW);
    digitalWrite(RED_LED_1, HIGH);
    WiFi.begin(ssid, pass);
  }

  isBlynkConnected = Blynk.connected();
  if (isBlynkConnected) {
    digitalWrite(GREEN_LED_1, HIGH);
    Blynk.virtualWrite(V0, "SYSTEM ACTIVE");
  } else {
    digitalWrite(GREEN_LED_1, LOW);
    if(isWiFiConnected) {
      Blynk.connect();
      Blynk.virtualWrite(V0, "SYSTEM OFFLINE");
    }
  }
}

void loop() {
  timer.run();
  
  if (WiFi.status() == WL_CONNECTED) {
    Blynk.run();
  }

  readMPU();

  // --- FIXED: Crack Detection Logic ---
  float currentDistance = getUltrasonicDistance();
  
  if (currentDistance > 0) {
    if (currentDistance > (baseDistance + crackThreshold)) {
      crackDetected = true;
    } else {
      crackDetected = false;
    }
  }

  Serial.print(" | Dist: "); Serial.print(currentDistance);
  Serial.println();

  delay(100);
}

void sendDataToBlynk() {
  if (Blynk.connected()) {
    Blynk.virtualWrite(V5, crackDetected ? 255 : 0);
  }
}


float getUltrasonicDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  // 26ms timeout avoids huge junk values if echo is missed
  long duration = pulseIn(ECHO_PIN, HIGH, 26000); 
  
  if (duration <= 0) return -1.0; 
  
  return (duration * SOUND_SPEED) / 2.0;
}
