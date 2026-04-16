//Sensors: MPU6050, HX711, HC-SR04
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <MPU6050.h>
#include <HX711.h>

char ssid[] = "YOUR_WIFI_SSID";
char pass[] = "YOUR_WIFI_PASSWORD";

char auth[] = "YOUR_BLYNK_AUTH_TOKEN";


BlynkTimer timer;

void setup() {
  Serial.begin(115200);
  
  // Initialize WiFi and Blynk
  Serial.println("Connecting to WiFi...");
  Serial.println(ssid);
  
  Blynk.begin(auth, ssid, pass);
  
  // Connection with timeout
  int attempts = 0;
  while (Blynk.connect() == false && attempts < 20) {
    Serial.print(".");
    delay(500);
    attempts++;
  }

  if (Blynk.connected()) {
    Serial.println("\nConnected to Blynk!");
    if (lcdInitialized) {
      lcdClear();
      lcdSetCursor(0, 0);
      lcdPrint("Connected!");
      lcdSetCursor(0, 1);
      lcdPrint("IP:" + WiFi.localIP().toString().substring(0, 12));
      delay(2000);
    }
  } else {
    Serial.println("Failed to connect!");
    if (lcdInitialized) {
      lcdClear();
      lcdSetCursor(0, 0);
      lcdPrint("Connection Fail");
      lcdSetCursor(0, 1);
      lcdPrint("Check Settings");
      delay(3000);
    }
  }

  Serial.println("Bridge Monitoring System Ready!");
  Blynk.virtualWrite(VPIN_ALERT_STATUS, "System Online");
}

void loop() {
  Blynk.run();
  timer.run();
}

// Blynk function to reset baseline (can be triggered from app)
BLYNK_WRITE(V11) {
  int resetValue = param.asInt();
  if (resetValue == 1) {
    calibrateMPU6050();
    scale.tare();
    baselineCrack1 = measureDistance(TRIG_PIN_1, ECHO_PIN_1);
    baselineCrack2 = measureDistance(TRIG_PIN_2, ECHO_PIN_2);
    Blynk.virtualWrite(VPIN_ALERT_STATUS, "Baseline Reset Complete");
    Serial.println("System recalibrated");
  }
}