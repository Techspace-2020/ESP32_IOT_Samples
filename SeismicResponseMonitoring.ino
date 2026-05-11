#define BLYNK_TEMPLATE_ID "TMPL3ab0XD9B2"
#define BLYNK_TEMPLATE_NAME "Seismic Monitoring System Mohit Kumar"
#define BLYNK_AUTH_TOKEN "gjl_aZjJpXwdQq_Lrdq5aFBcL8NQnLkj"

#include <Wire.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

// WiFi Credentials
char ssid[] = "JioFiber4G";
//char pass[] = "@Intelli_Aviotz$";
char pass[] = "Animal1234";

// Pin Definitions
#define MPU_ADDR 0x68
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

  // MPU6050 Initialization
  Wire.begin(21, 22);
  delay(200);
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0x00);
  Wire.endTransmission(true);
  Serial.println("MPU Initialized");

  delay(2000);
  calibrateIMU(); // Original logic preserved
  calibrateTemperature(); // Original logic preserved
  prevTime = millis();

  // Blynk & WiFi Config
  WiFi.begin(ssid, pass);
  Blynk.config(BLYNK_AUTH_TOKEN);
  
  // Timers: Check connection every 5s, send data every 1s
  timer.setInterval(5000L, checkConnections);
  timer.setInterval(1000L, sendDataToBlynk);

  Serial.println("System Ready\n");
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
  } else {
    digitalWrite(GREEN_LED_1, LOW);
    if(isWiFiConnected) {
      Blynk.connect();
    }
  }
  if(WiFi.status()==WL_CONNECTED && isBlynkConnected){
    Blynk.virtualWrite(V1, "SYSTEM ACTIVE");
  }else {
    Blynk.virtualWrite(V1, "SYSTEM OFFLINE");
  }
}

void loop() {
  timer.run();
  
  if (WiFi.status() == WL_CONNECTED) {
    Blynk.run();
  }

  // Core MPU Processing
  readMPU();

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

   // --- MOTION DETECTION ---
  float accel_mag = sqrt(ax_g*ax_g + ay_g*ay_g + az_g*az_g);
  bool motionDetected = false;

  if (fabs(accel_mag - 1.0) > accel_threshold ||
      fabs(gx_dps) > gyro_threshold ||
      fabs(gy_dps) > gyro_threshold ||
      fabs(gz_dps) > gyro_threshold) {
    motionDetected = true;
  }

  // if (motionDetected || abs(roll) > 45 || abs(pitch) > 45) {
  //   digitalWrite(RED_LED_1, HIGH);   
  //   tone(Buzzer, 500);             
    
  //   // Optional: Send emergency notification to Blynk
  // } else {
  //   // Return to normal (Red LED is also managed by checkConnections for WiFi status)
  //   // We only turn it off here if WiFi is actually connected
  //   if (isWiFiConnected) {
  //     digitalWrite(RED_LED_1, LOW); 
  //   }
  //   noTone(Buzzer);                  // Stop the buzzer
  // }

  if (motionDetected || abs(roll)>45) {
    tone(Buzzer,2000);
    digitalWrite(RED_LED_2, HIGH);
    digitalWrite(GREEN_LED_2, LOW);
  } else {
    noTone(Buzzer);
    digitalWrite(RED_LED_2, LOW);
    digitalWrite(GREEN_LED_2, HIGH);
  }

  Serial.print("Temp: ");
  Serial.print(temperature, 2);

  Serial.print(" | Roll: ");
  Serial.print(roll, 2);

  Serial.print(" Pitch: ");
  Serial.print(pitch, 2);

  Serial.print(" | Motion: ");
  Serial.print(motionDetected ? "YES" : "NO");

  Serial.println();

  delay(100);
}

// Function to push calculated data to Blynk Cloud
void sendDataToBlynk() {
  float ax_g = (ax - ax_off) / 16384.0;
  float ay_g = (ay - ay_off) / 16384.0;
  float az_g = (az - az_off) / 16384.0;
  float gx_dps = (gx - gx_off) / 131.0;
  float gy_dps = (gy - gy_off) / 131.0;
  float gz_dps = (gz - gz_off) / 131.0;
  
  float temperature = ((tempRaw / 340.0) + 36.53) - temp_offset;
  float accel_mag = sqrt(ax_g*ax_g + ay_g*ay_g + az_g*az_g);
  
  bool motion = false;
  if (fabs(accel_mag - 1.0) > accel_threshold || 
      fabs(gx_dps) > gyro_threshold || 
      fabs(gy_dps) > gyro_threshold || 
      fabs(gz_dps) > gyro_threshold) {
    motion = true;
  }

  if (Blynk.connected()) {
    Blynk.virtualWrite(V2, temperature);
    Blynk.virtualWrite(V3, roll);
    Blynk.virtualWrite(V4, pitch);
    Blynk.virtualWrite(V5, motion ? 255 : 0);
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
    ax_off += ax;
    ay_off += ay;
    az_off += (az - 16384);
    gx_off += gx;
    gy_off += gy;
    gz_off += gz;
    delay(5);
  }
  ax_off /= samples;
  ay_off /= samples;
  az_off /= samples;
  gx_off /= samples;
  gy_off /= samples;
  gz_off /= samples;
}

void calibrateTemperature() {
  long sum = 0;
  int samples = 100;
  for (int i = 0; i < samples; i++) {
    readMPU();
    sum += tempRaw;
    delay(10);
  }
  float avgRaw = sum / (float)samples;
  float measured = (avgRaw / 340.0) + 36.53;
  float assumed_room = 30.0;
  temp_offset = measured - assumed_room;
}
