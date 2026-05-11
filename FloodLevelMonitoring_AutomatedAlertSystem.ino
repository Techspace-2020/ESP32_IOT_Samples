#define BLYNK_TEMPLATE_ID "TMPL34bjMw5Wx"
#define BLYNK_TEMPLATE_NAME "Flood Level Monitoring Automated Alert System"
#define BLYNK_AUTH_TOKEN "WwjFQLa4Nv_I5sNSx0lxGvM0IHZvU7Jn"

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

// WiFi Credentials
// char ssid[] = "JioFiber4G";
// char pass[] = "Animal1234";
char ssid[] = "Rakesh Rocky";
char pass[] = "nu1rs59j4k";

// Pin Definitions - Indicators
#define RED_LED_1 4
#define RED_LED_2 5
#define BLUE_LED_1 2
#define BLUE_LED_2 16
#define GREEN_LED_1 0
#define GREEN_LED_2 17
#define Buzzer 19

// --- Pins from your Image ---
// We use 13, 12, 14, 27, 26, 25, 33 as our 7 levels.
// The GND pin in your image is the "Common" wire in the water.
const int levelPins[7] = {13, 12, 14, 27, 26, 25, 33}; 
int currentFloodLevel = 0;
bool flood_alert = false;

// Connection Flags
bool isWiFiConnected = false;
bool isBlynkConnected = false;
BlynkTimer timer;

void setup() {
  Serial.begin(115200);
  
  pinMode(RED_LED_1, OUTPUT);
  pinMode(RED_LED_2, OUTPUT);
  pinMode(BLUE_LED_1, OUTPUT);
  pinMode(BLUE_LED_2, OUTPUT);
  pinMode(GREEN_LED_1, OUTPUT);
  pinMode(GREEN_LED_2, OUTPUT);
  pinMode(Buzzer, OUTPUT);

  // Initialize the 7 levels with PULLUP. 
  // They stay HIGH normally and go LOW when the GND wire touches them via water.
  for (int i = 0; i < 7; i++) {
    pinMode(levelPins[i], INPUT_PULLUP);
  }

  WiFi.begin(ssid, pass);
  Blynk.config(BLYNK_AUTH_TOKEN);

  timer.setInterval(5000L, checkConnections);
  timer.setInterval(1000L, readWaterLevel); 
}

void loop() {
  timer.run();
  if (WiFi.status() == WL_CONNECTED) {
    Blynk.run();
  }

  if (flood_alert) {
    tone(Buzzer, 2000,200); 
    digitalWrite(RED_LED_2, HIGH);
    digitalWrite(GREEN_LED_2, LOW);
  } else {
    noTone(Buzzer);
    digitalWrite(RED_LED_2, LOW);
    digitalWrite(GREEN_LED_2, HIGH);
  }
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
    Blynk.virtualWrite(V1, "SYSTEM ACTIVE");
  } else {
    digitalWrite(GREEN_LED_1, LOW);
    if (isWiFiConnected) Blynk.connect();
    Blynk.virtualWrite(V1, "SYSTEM OFFLINE");
  }
}

void readWaterLevel() {
  int highPoint = 0;
  
  // Logic: Since the common wire is GND, the pins will read LOW when wet.
  for (int i = 0; i < 7; i++) {
    if (digitalRead(levelPins[i]) == LOW) {
      highPoint = i + 1; 
    }
  }

  currentFloodLevel = highPoint;
  
  // Trigger Buzzer at Level 7 (Pin 33)
  flood_alert = (currentFloodLevel >= 7);
  Blynk.virtualWrite(V3, flood_alert);

  if (isBlynkConnected) {
    Blynk.virtualWrite(V2, currentFloodLevel); 
  }
  
  Serial.print("Flood Level: ");
  Serial.println(currentFloodLevel);
}

