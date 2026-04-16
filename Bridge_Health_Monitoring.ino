//Sensors: MPU6050, HX711, HC-SR04
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <MPU6050.h>
#include <HX711.h>

char ssid[] = "YOUR_WIFI_SSID";
char pass[] = "YOUR_WIFI_PASSWORD";

char auth[] = "YOUR_BLYNK_AUTH_TOKEN";

MPU6050 mpu;

// HX711 Load Cell Configuration
#define LOADCELL_DOUT_PIN  16
#define LOADCELL_SCK_PIN   17
HX711 scale;

// Ultrasonic Sensors for Crack Detection
#define TRIG_PIN_1  25
#define ECHO_PIN_1  26
#define TRIG_PIN_2  27
#define ECHO_PIN_2  14

// Alert Thresholds
#define VIBRATION_THRESHOLD 2.0      // g-force
#define LOAD_THRESHOLD 5000.0        // kg
#define CRACK_THRESHOLD 5.0          // cm (expanding crack)
#define ANGULAR_VEL_THRESHOLD 50.0   // deg/s

// Blynk Virtual Pins
#define VPIN_VIBRATION_X    V0
#define VPIN_VIBRATION_Y    V1
#define VPIN_VIBRATION_Z    V2
#define VPIN_LOAD           V3
#define VPIN_CRACK_1        V4
#define VPIN_CRACK_2        V5
#define VPIN_GYRO_X         V6
#define VPIN_GYRO_Y         V7
#define VPIN_GYRO_Z         V8
#define VPIN_ALERT_STATUS   V9
#define VPIN_TEMPERATURE    V10

// Global Variables
float baselineAccel[3] = {0, 0, 0};
float baselineCrack1 = 0;
float baselineCrack2 = 0;
String alertMessage = "";

BlynkTimer timer;

void setup() {
  Serial.begin(115200);
  
  // Initialize WiFi and Blynk
  Serial.println("Connecting to WiFi...");
  Blynk.begin(auth, ssid, pass);
  
  // Initialize I2C for MPU6050
  Wire.begin();
  
  // Initialize MPU6050
  Serial.println("Initializing MPU6050...");
  mpu.initialize();
  if (!mpu.testConnection()) {
    Serial.println("MPU6050 connection failed!");
  } else {
    Serial.println("MPU6050 connected successfully");
    calibrateMPU6050();
  }
  
  // Initialize HX711 Load Cell
  Serial.println("Initializing HX711...");
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(2280.f); // Calibration factor (adjust based on your load cell)
  scale.tare(); // Reset to zero
  
  // Initialize Ultrasonic Sensors
  pinMode(TRIG_PIN_1, OUTPUT);
  pinMode(ECHO_PIN_1, INPUT);
  pinMode(TRIG_PIN_2, OUTPUT);
  pinMode(ECHO_PIN_2, INPUT);
  
  // Calibrate crack sensors (baseline measurement)
  baselineCrack1 = measureDistance(TRIG_PIN_1, ECHO_PIN_1);
  baselineCrack2 = measureDistance(TRIG_PIN_2, ECHO_PIN_2);
  
  // Setup timers
  timer.setInterval(1000L, readSensorsAndSend);
  timer.setInterval(5000L, checkAlerts);
  
  Serial.println("Bridge Monitoring System Ready!");
  Blynk.virtualWrite(VPIN_ALERT_STATUS, "System Online");
}

void loop() {
  Blynk.run();
  timer.run();
}

// Calibrate MPU6050 to get baseline readings
void calibrateMPU6050() {
  Serial.println("Calibrating MPU6050...");
  int16_t ax, ay, az, gx, gy, gz;
  long axSum = 0, aySum = 0, azSum = 0;
  int samples = 100;
  
  for (int i = 0; i < samples; i++) {
    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
    axSum += ax;
    aySum += ay;
    azSum += az;
    delay(10);
  }
  
  baselineAccel[0] = (float)axSum / samples / 16384.0;
  baselineAccel[1] = (float)aySum / samples / 16384.0;
  baselineAccel[2] = (float)azSum / samples / 16384.0;
  
  Serial.println("Calibration complete");
}

// Read all sensors and send to Blynk
void readSensorsAndSend() {
  // Read MPU6050 (Vibration and Angular Velocity)
  int16_t ax, ay, az, gx, gy, gz;
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
  
  // Convert to g-force (acceleration)
  float accelX = (float)ax / 16384.0 - baselineAccel[0];
  float accelY = (float)ay / 16384.0 - baselineAccel[1];
  float accelZ = (float)az / 16384.0 - baselineAccel[2];
  
  // Convert to degrees/second (gyroscope)
  float gyroX = (float)gx / 131.0;
  float gyroY = (float)gy / 131.0;
  float gyroZ = (float)gz / 131.0;
  
  // Calculate vibration magnitude
  float vibrationMag = sqrt(accelX*accelX + accelY*accelY + accelZ*accelZ);
  
  // Read Load Cell
  float load = scale.get_units(5); // Average of 5 readings
  if (load < 0) load = 0; // Ignore negative values
  
  // Read Ultrasonic Sensors (Crack Detection)
  float crack1 = measureDistance(TRIG_PIN_1, ECHO_PIN_1);
  float crack2 = measureDistance(TRIG_PIN_2, ECHO_PIN_2);
  
  // Calculate crack expansion
  float crackExpansion1 = abs(crack1 - baselineCrack1);
  float crackExpansion2 = abs(crack2 - baselineCrack2);
  
  // Read Temperature from MPU6050
  int16_t rawTemp = mpu.getTemperature();
  float temperature = (float)rawTemp / 340.0 + 36.53;
  
  // Send data to Blynk
  Blynk.virtualWrite(VPIN_VIBRATION_X, accelX);
  Blynk.virtualWrite(VPIN_VIBRATION_Y, accelY);
  Blynk.virtualWrite(VPIN_VIBRATION_Z, accelZ);
  Blynk.virtualWrite(VPIN_LOAD, load);
  Blynk.virtualWrite(VPIN_CRACK_1, crackExpansion1);
  Blynk.virtualWrite(VPIN_CRACK_2, crackExpansion2);
  Blynk.virtualWrite(VPIN_GYRO_X, gyroX);
  Blynk.virtualWrite(VPIN_GYRO_Y, gyroY);
  Blynk.virtualWrite(VPIN_GYRO_Z, gyroZ);
  Blynk.virtualWrite(VPIN_TEMPERATURE, temperature);
  
  // Debug output
  Serial.printf("Vibration: %.2f g | Load: %.2f kg | Crack1: %.2f cm | Crack2: %.2f cm\n", 
                vibrationMag, load, crackExpansion1, crackExpansion2);
  Serial.printf("Gyro - X:%.2f Y:%.2f Z:%.2f deg/s | Temp: %.2f°C\n", 
                gyroX, gyroY, gyroZ, temperature);
}

// Measure distance using ultrasonic sensor
float measureDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  long duration = pulseIn(echoPin, HIGH, 30000); // Timeout 30ms
  if (duration == 0) return -1; // Sensor error
  
  float distance = duration * 0.034 / 2; // Convert to cm
  return distance;
}

// Check for alert conditions
void checkAlerts() {
  alertMessage = "";
  bool alertTriggered = false;
  
  // Read current sensor values
  int16_t ax, ay, az, gx, gy, gz;
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
  
  float accelX = (float)ax / 16384.0 - baselineAccel[0];
  float accelY = (float)ay / 16384.0 - baselineAccel[1];
  float accelZ = (float)az / 16384.0 - baselineAccel[2];
  float vibrationMag = sqrt(accelX*accelX + accelY*accelY + accelZ*accelZ);
  
  float gyroX = abs((float)gx / 131.0);
  float gyroY = abs((float)gy / 131.0);
  float gyroZ = abs((float)gz / 131.0);
  
  float load = scale.get_units(5);
  float crack1 = abs(measureDistance(TRIG_PIN_1, ECHO_PIN_1) - baselineCrack1);
  float crack2 = abs(measureDistance(TRIG_PIN_2, ECHO_PIN_2) - baselineCrack2);
  
  // Check vibration threshold
  if (vibrationMag > VIBRATION_THRESHOLD) {
    alertMessage += "⚠ HIGH VIBRATION! ";
    alertTriggered = true;
  }
  
  // Check load threshold
  if (load > LOAD_THRESHOLD) {
    alertMessage += "⚠ OVERLOAD! ";
    alertTriggered = true;
  }
  
  // Check crack expansion
  if (crack1 > CRACK_THRESHOLD || crack2 > CRACK_THRESHOLD) {
    alertMessage += "⚠ CRACK EXPANSION DETECTED! ";
    alertTriggered = true;
  }
  
  // Check angular velocity (twisting/turning)
  if (gyroX > ANGULAR_VEL_THRESHOLD || gyroY > ANGULAR_VEL_THRESHOLD || gyroZ > ANGULAR_VEL_THRESHOLD) {
    alertMessage += "⚠ EXCESSIVE DEFLECTION! ";
    alertTriggered = true;
  }
  
  // Send alert to Blynk
  if (alertTriggered) {
    Blynk.virtualWrite(VPIN_ALERT_STATUS, alertMessage);
    Blynk.logEvent("bridge_alert", alertMessage); // Requires Blynk event setup
    Serial.println("ALERT: " + alertMessage);
  } else {
    Blynk.virtualWrite(VPIN_ALERT_STATUS, "All systems normal");
  }
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