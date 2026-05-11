#define BLYNK_TEMPLATE_ID "TMPL3czJnlmzo"
#define BLYNK_TEMPLATE_NAME "Bridge Health Monitoring Shreyas"
#define BLYNK_AUTH_TOKEN "SNkEqQ0cNqoatX91OtsrP_5WF4cJ7hNR"

#include <Wire.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

// WiFi Credentials
char ssid[] = "Rakesh Rocky";
char pass[] = "nu1rs59j4k";

// MPU6250/6500
#define MPU_ADDR 0x68
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

// MPU6050 Variables
int16_t ax, ay, az, gx, gy, gz, tempRaw;
float ax_off = 0, ay_off = 0, az_off = 0;
float gx_off = 0, gy_off = 0, gz_off = 0;
float temp_offset = 0;
float roll = 0, pitch = 0;
unsigned long prevTime = 0;

float crackThreshold = 3.0; 
float baseDistance = 0;
bool crackDetected = false;

// Motion detection
float accel_threshold = 0.2; 
float gyro_threshold = 20.0;

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

  // MPU6050 Initialization
  Wire.begin(21, 22);
  delay(200);
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0x00);
  Wire.endTransmission(true);
  Serial.println("MPU Initialized");

  delay(2000);
  calibrateIMU(); 
  calibrateTemperature();

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

  unsigned long currentTime = millis();
  float dt = (currentTime - prevTime) / 1000.0;
  prevTime = currentTime;

  float ax_g = (ax - ax_off) / 16384.0;
  float ay_g = (ay - ay_off) / 16384.0;
  float az_g = (az - az_off) / 16384.0;
  float gx_dps = (gx - gx_off) / 131.0;
  float gy_dps = (gy - gy_off) / 131.0;
  float gz_dps = (gz - gz_off) / 131.0;

  float temperature = ((tempRaw / 340.0) + 36.53) - temp_offset;

  float roll_acc = atan2(ay_g, az_g) * 57.3;
  float pitch_acc = atan2(-ax_g, sqrt(ay_g * ay_g + az_g * az_g)) * 57.3;

  float alpha = 0.98;
  roll = alpha * (roll + gx_dps * dt) + (1 - alpha) * roll_acc;
  pitch = alpha * (pitch + gy_dps * dt) + (1 - alpha) * pitch_acc;

  float accel_mag = sqrt(ax_g*ax_g + ay_g*ay_g + az_g*az_g);
  bool motionDetected = false;

  if (fabs(accel_mag - 1.0) > accel_threshold ||
      fabs(gx_dps) > gyro_threshold ||
      fabs(gy_dps) > gyro_threshold ||
      fabs(gz_dps) > gyro_threshold) {
    motionDetected = true;
  }

  // Updated Alarm Logic to include crack detection
  if (motionDetected || abs(roll) > 45 || crackDetected) {
    tone(Buzzer, 2000);
    digitalWrite(RED_LED_2, HIGH);
    digitalWrite(GREEN_LED_2, LOW);
  } else {
    noTone(Buzzer);
    digitalWrite(RED_LED_2, LOW);
    digitalWrite(GREEN_LED_2, HIGH);
  }

  // if(crackDetected){
  //   tone(Buzzer, 2000);
  //   digitalWrite(RED_LED_2, HIGH);
  //   digitalWrite(GREEN_LED_2, LOW);
  // }else{
  //   noTone(Buzzer);
  //   digitalWrite(RED_LED_2, LOW);
  //   digitalWrite(GREEN_LED_2, HIGH);
  // }

  // Serial Monitor
  Serial.print("Temp: "); Serial.print(temperature, 2);
  Serial.print(" | Roll: "); Serial.print(roll, 2);
  Serial.print(" | Pitch: "); Serial.print(pitch, 2);
  Serial.print(" | Motion: "); Serial.print(motionDetected ? "YES" : "NO");
  Serial.print(" | Crack: "); Serial.print(crackDetected ? "YES" : "NO");
  Serial.print(" | Dist: "); Serial.print(currentDistance);
  Serial.println();

  delay(100);
}

void sendDataToBlynk() {
  float ax_g = (ax - ax_off) / 16384.0;
  float ay_g = (ay - ay_off) / 16384.0;
  float az_g = (az - az_off) / 16384.0;
  float gx_dps = (gx - gx_off) / 131.0;
  float gy_dps = (gy - gy_off) / 131.0;
  float gz_dps = (gz - gz_off) / 131.0;
  float accel_mag = sqrt(ax_g*ax_g + ay_g*ay_g + az_g*az_g);

  float temperature = ((tempRaw / 340.0) + 36.53) - temp_offset;
  bool motionDetected = false;
  if (fabs(accel_mag - 1.0) > accel_threshold ||
      fabs(gx_dps) > gyro_threshold ||
      fabs(gy_dps) > gyro_threshold ||
      fabs(gz_dps) > gyro_threshold) {
    motionDetected = true;
  }

  if (Blynk.connected()) {
    Blynk.virtualWrite(V1, temperature);
    Blynk.virtualWrite(V2, roll);
    Blynk.virtualWrite(V3, pitch);
    Blynk.virtualWrite(V4, motionDetected ? 255 : 0);
    Blynk.virtualWrite(V5, crackDetected ? 255 : 0);
  }
}

void readMPU() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 14, true);
  ax = Wire.read() << 8 | Wire.read();
  ay = Wire.read() << 8 | Wire.read();
  az = Wire.read() << 8 | Wire.read();
  tempRaw = Wire.read() << 8 | Wire.read();
  gx = Wire.read() << 8 | Wire.read();
  gy = Wire.read() << 8 | Wire.read();
  gz = Wire.read() << 8 | Wire.read();
}

void calibrateIMU() {
  int samples = 500;
  for (int i = 0; i < samples; i++) {
    readMPU();
    ax_off += ax; ay_off += ay; az_off += (az - 16384);
    gx_off += gx; gy_off += gy; gz_off += gz;
    delay(5);
  }
  ax_off /= samples; ay_off /= samples; az_off /= samples;
  gx_off /= samples; gy_off /= samples; gz_off /= samples;
}

void calibrateTemperature() {
  long sum = 0;
  for (int i = 0; i < 100; i++) {
    readMPU(); sum += tempRaw; delay(10);
  }
  float avgRaw = sum / 100.0;
  float measured = (avgRaw / 340.0) + 36.53;
  temp_offset = measured - 30.0;
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
