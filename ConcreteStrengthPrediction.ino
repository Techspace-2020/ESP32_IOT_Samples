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

// Hardware PIN Definitions from Core UPV Logic
#define TX1 26
#define TX2 27
#define ECHO_PIN 34
#define SOUND_SPEED 0.034

#define RED_LED_1 4
#define RED_LED_2 5
#define BLUE_LED_1 2
#define BLUE_LED_2 16
#define GREEN_LED_1 0
#define GREEN_LED_2 17
#define Buzzer 19

#define PROBE_DISTANCE_M 0.10

// Global variables for UPV Interrupt handling
volatile uint32_t echo_time = 0;
volatile bool echo_received = false;
volatile uint32_t start_time = 0;

// Variables for calculated data to send to Blynk
float avg_us = 0;
float velocity = 0;
float strength = 0;
String quality = "UNKNOWN";

// float crackThreshold = 3.0; 
// float baseDistance = 0;
// bool crackDetected = false;

// Connection Flags & Timer
bool isWiFiConnected = false;
bool isBlynkConnected = false;
BlynkTimer timer;

// High-speed Hardware Interrupt Service Routine executed directly from internal RAM
void IRAM_ATTR echoISR() {
  if (!echo_received) {
    echo_time = micros() - start_time;
    echo_received = true;
  }
}

// Generates a 10-cycle 41.6kHz differential push-pull acoustic pulse train
void sendBurst() {
  for (int i = 0; i < 10; i++) {
    digitalWrite(TX1, HIGH);
    digitalWrite(TX2, LOW);
    delayMicroseconds(12);
    digitalWrite(TX1, LOW);
    digitalWrite(TX2, HIGH);
    delayMicroseconds(12);
  }
  digitalWrite(TX1, LOW);
  digitalWrite(TX2, LOW);
}

void setup() {
  Serial.begin(115200);

  // LED & Buzzer Setup
  pinMode(RED_LED_1, OUTPUT);
  pinMode(RED_LED_2, OUTPUT);
  pinMode(BLUE_LED_1, OUTPUT);
  pinMode(GREEN_LED_1, OUTPUT);
  pinMode(GREEN_LED_2, OUTPUT);
  pinMode(Buzzer, OUTPUT);
  pinMode(TX1, OUTPUT);
  pinMode(TX2, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  tone(Buzzer, 2000, 200);

  delay(2000);

  // Connect falling/rising digital edge changes to high-speed interrupt block
  attachInterrupt(ECHO_PIN, echoISR, RISING);
  
  // Initialization alert signal to confirm circuit stability
  digitalWrite(Buzzer, HIGH);
  delay(100);
  digitalWrite(Buzzer, LOW);
  
  Serial.println("UPV Concrete System Initialized");

  // Initialize Network Connections
  WiFi.begin(ssid, pass);
  Blynk.config(BLYNK_AUTH_TOKEN);
  
  // Setup Timers (Replaced blocking delay loop with a non-blocking 4-second sensor poll)
  timer.setInterval(5000L, checkConnections);
  timer.setInterval(4000L, performUPVAnalysis);
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
}

void sendDataToBlynk() {
  if (Blynk.connected()) {
    // Map your custom metrics to clean Virtual Pins on your dashboard
    Blynk.virtualWrite(V1, avg_us);     // Average Time of Flight (us)
    Blynk.virtualWrite(V2, velocity);   // Ultrasonic Wave Velocity (m/s)
    Blynk.virtualWrite(V3, strength);   // Predicted Compressive Strength (MPa)
    Blynk.virtualWrite(V4, quality);    // Qualitative Material Classification
  }
}


// core UPV logic isolated into a clean timer function to protect WiFi/Blynk execution
void performUPVAnalysis() {
  const int samples = 10;
  uint32_t total_time = 0;
  int valid = 0;

  for (int s = 0; s < samples; s++) {
    echo_received = false;

    sendBurst();
    start_time = micros();

    // Asynchronous listening window wrapper with a 5000 microsecond limit
    uint32_t timeout = micros();
    while (!echo_received && (micros() - timeout < 5000)) {}

    if (echo_received) {
      Serial.print("Sample [");
      Serial.print(s);
      Serial.print("] Verified TOF: ");
      Serial.print(echo_time);
      Serial.println(" us");

      // Apply digital band filter boundaries to reject cross-talk and shear waves
      if (echo_time > 10 && echo_time < 5000) {
        total_time += echo_time;
        valid++;
      }
    } else {
      Serial.print("Sample [");
      Serial.print(s);
      Serial.println("] Error: Echo Signal Missing");
    }
    delay(60);
  }

  // Finalize batch analysis and execute mathematical conversion structures
  if (valid > 0) {
    avg_us = (float)total_time / valid;
    
    // Velocity kinematic model: Distance divided by Time parameter
    velocity = PROBE_DISTANCE_M / (avg_us / 1e6);

    // Exponential regression algorithm for concrete strength prediction
    strength = 1.42 * exp(0.00083 * velocity);

    Serial.println("=========================================");
    Serial.print("Calculated Average TOF : "); 
    Serial.print(avg_us, 1); 
    Serial.println(" us");
    Serial.print("Computed Wave Velocity : "); 
    Serial.print(velocity, 0); 
    Serial.println(" m/s");
    Serial.print("Predicted Comp. Strength: "); 
    Serial.print(strength, 2); 
    Serial.println(" MPa");

    // Structural material classification logic block
    if (velocity > 4500) {
      quality = "EXCELLENT";
      digitalWrite(GREEN_LED_1, HIGH); // Confirm structural readiness
    } else if (velocity > 3500) {
      quality = "GOOD";
      digitalWrite(GREEN_LED_2, HIGH); // Confirm structural readiness
    } else if (velocity > 3000) {
      quality = "MEDIUM";
      digitalWrite(BLUE_LED_1, LOW);
    } else if (velocity > 2000) {
      quality = "DOUBTFUL";
      digitalWrite(BLUE_LED_2, LOW);
    } else {
      quality = "POOR";
      digitalWrite(RED_LED_1, LOW);
    }

    Serial.print("Material Classification: "); 
    Serial.println(quality);
    Serial.println("=========================================");
  } else {
    Serial.println("Data Error: No Valid Acoustic Profiles Isolated.");
    quality = "ERROR";
    
    // Alert signal sequence to notify site crew of error status
    digitalWrite(Buzzer, HIGH);
    delay(500);
    digitalWrite(Buzzer, LOW);
  }
}
