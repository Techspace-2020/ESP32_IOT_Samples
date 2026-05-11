#define BLYNK_TEMPLATE_ID "TMPL3iT1j4lff"
#define BLYNK_TEMPLATE_NAME "Strom Water Drainage Blockage detection Bharath"
#define BLYNK_AUTH_TOKEN "di5gxgg90-9iEIE2gN_8YG6XqhGROMGt"

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

// WiFi
// char ssid[] = "NANDY_STA_UP";
// char pass[] = "@Intelli_Aviotz$";
char ssid[] = "Rakesh Rocky";
char pass[] = "nu1rs59j4k";

#define RED_LED_1 4
#define RED_LED_2 5

#define BLUE_LED_1 2
#define BLUE_LED_2 16

#define GREEN_LED_1 0
#define GREEN_LED_2 17
#define BUZZER 19

// Pumps
// #define IN1 33
// #define IN2 25
// #define IN3 12
// #define IN4 13
#define IN1 12
#define IN2 13
#define IN3 33
#define IN4 25

// Flow Sensor
#define FLOW_SENSOR 32

// Water Level
#define LOW_PIN 14
#define MID_PIN 27
#define FULL_PIN 26

// Flow variables
volatile byte pulseCount = 0;
volatile unsigned long lastPulseTime = 0;

float flowRate = 0;
float calibrationFactor = 4.5;

unsigned long previousMillis = 0;

// Moving Average (10 samples)
float flowHistory[10] = {0};
int flowIndex = 0;

bool isBlynkConnected = false;
bool flow_motor_flag = false;
bool isWiFiConnected = false;
BlynkTimer timer;

// Global Control Variables
int btnMotor1 = 0; // Linked to V1
int btnMotor2 = 0; // Linked to V3
int currentLevel = 0;

// Button Widget for Motor 1 (Fill)
BLYNK_WRITE(V1) {
  btnMotor1 = param.asInt();
}
// Button Widget for Motor 2 (Drain/Flow)
BLYNK_WRITE(V3) {
  btnMotor2 = param.asInt();
}

// ---------------- INTERRUPT ----------------
void IRAM_ATTR pulseCounter() {
  unsigned long now = micros();

  if (now - lastPulseTime > 500) { // debounce
    pulseCount++;
    lastPulseTime = now;
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
  tone(BUZZER, 2000, 200);
  pinMode(BUZZER, OUTPUT);
  
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  pinMode(FLOW_SENSOR, INPUT_PULLDOWN);

  pinMode(LOW_PIN, INPUT_PULLUP);
  pinMode(MID_PIN, INPUT_PULLUP);
  pinMode(FULL_PIN, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR), pulseCounter, FALLING);

  WiFi.begin(ssid, pass);
  Blynk.config(BLYNK_AUTH_TOKEN);

  // Safe start
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);

  timer.setInterval(3000L, checkConnections);
  timer.setInterval(1000L, controlSystem);
  timer.setInterval(1000L, readFlow);
}

// ---------------- CONNECTION ----------------
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

// ---------------- WATER LEVEL + PUMPS ----------------
void controlSystem() {

  int low = digitalRead(LOW_PIN);
  int mid = digitalRead(MID_PIN);
  int full = digitalRead(FULL_PIN);

  int currentLevel = 0;
  String levelText = "EMPTY";

  if (full == LOW) {
    currentLevel = 100;
    levelText = "FULL";
  }
  else if (mid == LOW) {
    currentLevel = 50;
    levelText = "MEDIUM";
  }
  else if (low == LOW) {
    currentLevel = 25;
    levelText = "LOW";
  }

  Serial.print("Water Level: ");
  Serial.println(levelText);

  // Pump1 (fill)
  if (btnMotor1 == 1) {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
  } else {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
  }

  // --- Motor 2 Control (Drain Tank + Logic) ---
  // Conditions: Manual Switch is ON AND Water Level is 25% or higher
  if (btnMotor2 == 1 && currentLevel >= 25) {
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
  } else {
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
  }

  if (isBlynkConnected) {
    Blynk.virtualWrite(V8, levelText);
    Blynk.virtualWrite(V9, currentLevel);
  }
}

// ---------------- FLOW + FILTER ----------------
void readFlow() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= 1000) {
    float rawFlow = ((1000.0 / (currentMillis - previousMillis)) * pulseCount) / calibrationFactor;
    pulseCount = 0;
    previousMillis = currentMillis;

    flowHistory[flowIndex] = rawFlow;
    flowIndex = (flowIndex + 1) % 10;

    float sum = 0;
    for (int i = 0; i < 10; i++) sum += flowHistory[i];
    flowRate = sum / 10.0;
    if (flowRate < 0.1) flowRate = 0;

    // --- Blockage Logic ---
    bool motor2Active = digitalRead(IN3);

    // If Motor 2 should be running but flow is near zero, it's a blockage
    if (motor2Active && flowRate < 0.2) {
      digitalWrite(RED_LED_2, HIGH);
      tone(BUZZER, 1000);
      if (isBlynkConnected) {
        Blynk.virtualWrite(V6, 255); // LED Widget
        Blynk.virtualWrite(V7, "BLOCKAGE DETECTED");
      }
    } else {
      digitalWrite(RED_LED_2, LOW);
      noTone(BUZZER);
      if (isBlynkConnected) {
        Blynk.virtualWrite(V6, 0);
        Blynk.virtualWrite(V7, "NORMAL");
      }
    }

    if (isBlynkConnected) Blynk.virtualWrite(V2, flowRate);
  }
}

// ---------------- LOOP ----------------
void loop() {
  timer.run();

  if (WiFi.status() == WL_CONNECTED) {
    Blynk.run();
  }
}
