#define BLYNK_TEMPLATE_ID "TMPL3Da-v1snl"
#define BLYNK_TEMPLATE_NAME "Water Distrubution Leakage Detection Sumanth"
#define BLYNK_AUTH_TOKEN "9V2n-yB9mXcQXk4eG7BQGqsuMRAtNsGS"

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

#define RED_LED_1 4
#define RED_LED_2 5

#define BLUE_LED_1 2
#define BLUE_LED_2 16

#define GREEN_LED_1 0
#define GREEN_LED_2 17

#define Buzzer 19

#define IN1 26
#define IN2 27

#define water_sensor1 14
#define water_sensor2 13

// WiFi Credentials
// char ssid[] = "JioFiber4G";
// char pass[] = "Animal1234 ";
char ssid[] = "Rakesh Rocky";
char pass[] = "nu1rs59j4k";

long currentMillis = 0;
long previousMillis = 0;
int interval = 1000;
boolean ledState = LOW;
float calibrationFactor = 4.5;
volatile byte pulseCount1, pulseCount2;
byte pulse1Sec1 = 0, pulse1Sec2 = 0;
float flowRate1, flowRate2;
unsigned int flowMilliLitres1, flowMilliLitres2;
unsigned long totalMilliLitres1, totalMilliLitres2;

// Connection Flags
bool isWiFiConnected = false;
bool isBlynkConnected = false;
bool alert_flag = false;
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

void IRAM_ATTR pulseCounter_1() {
  pulseCount1++;
}

void IRAM_ATTR pulseCounter_2() {
  pulseCount2++;
}

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
   Serial.begin(115200);
  pinMode(RED_LED_1, OUTPUT);
  pinMode(RED_LED_2, OUTPUT);
  pinMode(BLUE_LED_1, OUTPUT);
  pinMode(BLUE_LED_2, OUTPUT);
  pinMode(GREEN_LED_1, OUTPUT);
  pinMode(GREEN_LED_2, OUTPUT);
  tone(Buzzer, 2000, 200);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  
  pinMode(Buzzer, OUTPUT);
  pinMode(water_sensor1, INPUT_PULLUP);
  pinMode(water_sensor2, INPUT_PULLUP);
  
  WiFi.begin(ssid, pass);
  Blynk.config(BLYNK_AUTH_TOKEN);
  timer.setInterval(5000L, checkConnections);
  timer.setInterval(1000L, readSensors);
  //timer.setInterval(500L, alertSystem);

  pulseCount1 = 0;
  flowRate1 = 0.0;
  flowMilliLitres1 = 0;
  totalMilliLitres1 = 0;
  previousMillis = 0;
  pulseCount2 = 0;
  flowRate2 = 0.0;
  flowMilliLitres2 = 0;
  totalMilliLitres2 = 0;

  attachInterrupt(digitalPinToInterrupt(water_sensor1), pulseCounter_1, FALLING);
  attachInterrupt(digitalPinToInterrupt(water_sensor2), pulseCounter_2, FALLING);
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
}

// the loop function runs over and over again forever
void loop() {
  // tone(Buzzer, 2000); // buzzer ON
  // digitalWrite(RED_LED_1, HIGH);
  // digitalWrite(RED_LED_2, HIGH);
  // digitalWrite(BLUE_LED_1, LOW);
  // digitalWrite(BLUE_LED_2, LOW);
  // digitalWrite(GREEN_LED_1, LOW);
  // digitalWrite(GREEN_LED_2, LOW);
  // digitalWrite(IN1, HIGH);
  // digitalWrite(IN2, LOW);

  // delay(1000); // wait for a second

  // digitalWrite(RED_LED_1, LOW);
  // digitalWrite(RED_LED_2, LOW);
  // digitalWrite(BLUE_LED_1, HIGH);
  // digitalWrite(BLUE_LED_2, HIGH);
  // digitalWrite(GREEN_LED_1, LOW);
  // digitalWrite(GREEN_LED_2, LOW);
  // delay(1000);
  // digitalWrite(RED_LED_1, LOW);
  // digitalWrite(RED_LED_2, LOW);
  // digitalWrite(BLUE_LED_1, LOW);
  // digitalWrite(BLUE_LED_2, LOW);
  // digitalWrite(GREEN_LED_1, HIGH);
  // digitalWrite(GREEN_LED_2, HIGH);

  // delay(1000); // wait for a second

  // digitalWrite(IN1, LOW);
  // digitalWrite(IN2, LOW);
  // // noTone(Buzzer); // buzzer OFF
  // delay(5000); // wait for a second

  // currentMillis = millis();
  // if (currentMillis - previousMillis > interval) {

  //   pulse1Sec1 = pulseCount1;
  //   pulseCount1 = 0;

  //   pulse1Sec2 = pulseCount2;
  //   pulseCount2 = 0;

  //   flowRate1 = ((1000.0 / (millis() - previousMillis)) * pulse1Sec1) / calibrationFactor;
  //   flowRate2 = ((1000.0 / (millis() - previousMillis)) * pulse1Sec2) / calibrationFactor;
  //   previousMillis = millis();

  //   flowMilliLitres1 = (flowRate1 / 60) * 1000;
  //   flowMilliLitres2 = (flowRate2 / 60) * 1000;

  //   // Add the millilitres passed in this second to the cumulative total
  //   totalMilliLitres1 += flowMilliLitres1;
  //   totalMilliLitres2 += flowMilliLitres2;

  //   // Print the flow rate for this second in litres / minute
  //   Serial.print("Flow rate1: ");
  //   Serial.print(int(flowRate1)); // Print the integer part of the variable
  //   Serial.print(" Flow rate2: ");
  //   Serial.print(int(flowRate2)); // Print the integer part of the variable
  //   Serial.print("L/min");
  //   Serial.print("\t"); // Print tab space

  //   // Print the cumulative total of litres flowed since starting
  //   Serial.print("Output Liquid Quantity: ");
  //   Serial.print(totalMilliLitres1);
  //   Serial.print("mL / ");
  //   Serial.print(totalMilliLitres1 / 1000);
  //   Serial.println("L ");
  //   Serial.print(totalMilliLitres2);
  //   Serial.print("mL / ");
  //   Serial.print(totalMilliLitres2 / 1000);
  //   Serial.println("L");

  //   // // Print the cumulative total of litres flowed since starting
  //   // Serial.print("Output Liquid Quantity2: ");
  //   // Serial.print(totalMilliLitres1);
  //   // Serial.print("mL / ");
  //   // Serial.print(totalMilliLitres2 / 1000);
  //   // Serial.println("L");
  // }
  timer.run();
  if (WiFi.status() == WL_CONNECTED) {
    Blynk.run();
  }
}

void readSensors(){
  currentMillis = millis();
  if (currentMillis - previousMillis > interval) {

    pulse1Sec1 = pulseCount1;
    pulseCount1 = 0;

    pulse1Sec2 = pulseCount2;
    pulseCount2 = 0;

    flowRate1 = ((1000.0 / (millis() - previousMillis)) * pulse1Sec1) / calibrationFactor;
    flowRate2 = ((1000.0 / (millis() - previousMillis)) * pulse1Sec2) / calibrationFactor;
    previousMillis = millis();

    flowMilliLitres1 = (flowRate1 / 60) * 1000;
    flowMilliLitres2 = (flowRate2 / 60) * 1000;

    // Add the millilitres passed in this second to the cumulative total
    totalMilliLitres1 += flowMilliLitres1;
    totalMilliLitres2 += flowMilliLitres2;

    // --- STEP 1: SEND DATA TO BLYNK ---
    // Mapping: V2=Flow1, V3=Flow2, V4=Total1, V5=Total2
    if (isBlynkConnected) {
      Blynk.virtualWrite(V2, flowRate1);
      Blynk.virtualWrite(V3, flowRate2);
      Blynk.virtualWrite(V4, totalMilliLitres1 / 1000.0); // Send in Liters
      Blynk.virtualWrite(V5, totalMilliLitres2 / 1000.0); // Send in Liters
    }

    // --- STEP 2: LEAKAGE DETECTION LOGIC ---
    // If flow1 is active but flow2 is significantly lower, assume a leak.
    // Threshold set to 0.5 L/min difference for sensitivity.
    if (flowRate1 > (flowRate2 + 0.5)) {
      alert_flag = true;
      digitalWrite(RED_LED_2, HIGH);
      digitalWrite(GREEN_LED_2, LOW);
      tone(Buzzer, 1000); // Alarm sound
      
      if (isBlynkConnected) {
        //Blynk.logEvent("leak_alert", "Warning: Leak detected in distribution!");
        Blynk.virtualWrite(V6, 255); // High intensity on a widget LED
      }
    } else {
      alert_flag = false;
      digitalWrite(RED_LED_2, LOW);
      digitalWrite(GREEN_LED_2, HIGH);
      noTone(Buzzer);
      
      if (isBlynkConnected) {
        Blynk.virtualWrite(V6, 0); // Turn off widget LED
      }
    }

    // Print the flow rate for this second in litres / minute
    Serial.print("Flow rate1: ");
    Serial.print(int(flowRate1)); // Print the integer part of the variable
    Serial.print(" Flow rate2: ");
    Serial.print(int(flowRate2)); // Print the integer part of the variable
    Serial.print("L/min");
    Serial.print("\t"); // Print tab space

    // Print the cumulative total of litres flowed since starting
    Serial.print("Output Liquid Quantity: ");
    Serial.print(totalMilliLitres1);
    Serial.print("mL / ");
    Serial.print(totalMilliLitres1 / 1000);
    Serial.println("L ");
    Serial.print(totalMilliLitres2);
    Serial.print("mL / ");
    Serial.print(totalMilliLitres2 / 1000);
    Serial.println("L");

    // // Print the cumulative total of litres flowed since starting
    // Serial.print("Output Liquid Quantity2: ");
    // Serial.print(totalMilliLitres1);
    // Serial.print("mL / ");
    // Serial.print(totalMilliLitres2 / 1000);
    // Serial.println("L");
  }
}

void alertSystem() {
  // Define a small threshold to account for sensor inaccuracy (e.g., 0.5 L/min)
  float threshold = 0.5;

  // Check if Inlet flow is significantly higher than Outlet flow
  if (flowRate1 > (flowRate2 + threshold)) {
    alert_flag = true;
    
    // Physical Alerts
    digitalWrite(RED_LED_2, HIGH);   // Turn on Red Alert LED
    digitalWrite(GREEN_LED_2, LOW);  // Turn off Normal Green LED
    tone(Buzzer, 1000);              // Constant alarm tone
    
    // Send to Blynk
    if (isBlynkConnected) {
      Blynk.virtualWrite(V6, 255);            // Turn on a Red LED widget on V6
      Blynk.virtualWrite(V7, "LEAK DETECTED"); // Status message on V7
      //Blynk.logEvent("leak_alert", "Water Leakage Detected in the System!");
    }
    
    Serial.println("--- ALERT: LEAKAGE DETECTED ---");
  } 
  else {
    alert_flag = false;
    
    // Physical Indicators (Normal State)
    digitalWrite(RED_LED_2, LOW);
    digitalWrite(GREEN_LED_2, HIGH);
    noTone(Buzzer);
    
    // Update Blynk
    if (isBlynkConnected) {
      Blynk.virtualWrite(V6, 0);               // Turn off LED widget
      Blynk.virtualWrite(V7, "FLOW NORMAL");    // Status message
    }
  }
}
