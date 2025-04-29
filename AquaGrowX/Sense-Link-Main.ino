#define BLYNK_TEMPLATE_ID "your_template_id"  // Define the Blynk template ID
#define BLYNK_TEMPLATE_NAME "your_template_name"  // Define the Blynk template name
#define BLYNK_AUTH_TOKEN "your_auth_token"  // Define the Blynk authentication token
#define BLYNK_PRINT Serial  // Enable serial print debugging

#include <ESP8266WiFi.h>  // Include ESP8266 Wi-Fi library
#include <DHT.h>  // Include DHT sensor library
#include <BlynkSimpleEsp8266.h>  // Include Blynk ESP8266 library
#include <Adafruit_Sensor.h>  // Include Adafruit sensor library

const char* ssid = "your_ssid";  // Wi-Fi SSID
const char* password = "your_password";  // Wi-Fi password

// Define sensor and actuator pins
const int tdsPin = A0;  
const int tempPin = D2;  
const int ultraSonicTrig = D0;  
const int ultraSonicEcho = D1;  
const int fanPin = D7;  
const int npumpPin = D3;  
const int wpumpPin = D5;  
const int airpumpPin = D6;  

#define DHTTYPE DHT11  // Define DHT sensor type
DHT dht(tempPin, DHTTYPE);  
long duration;

// Define thresholds and intervals for monitoring
const float thresholdTDS = 400;  // TDS threshold in ppm
const int tempFanOn = 30;  // Temperature threshold to turn fan on
const int tempFanOff = 20;  // Temperature threshold to turn fan off
const int maxDistance = 35;  // Maximum water level distance in cm

unsigned long lastLoopTime = millis();  
const unsigned long inter = 5 * 60 * 60 * 1000;  // Pump activation interval
const long interval = 60 * 60 * 1000;  // Air pump cycle interval
const unsigned long npumpInterval = 300000;  // Nutrient pump interval

unsigned long previousMillis = 0;  
unsigned long preMillis = 0;  
bool airpumpState = LOW;  

BlynkTimer timer;  
WiFiClient espClient;  

// Function to check and maintain TDS level
void checkTDS() {  
    unsigned long currMillis = millis();  
    if (currMillis - previousMillis >= npumpInterval) {  
        previousMillis = currMillis;  
        int tdsValue = analogRead(tdsPin);  
        float tdsvoltage = tdsValue * (3.3 / 1023.0);  
        float tds = (133.42 * tdsvoltage * tdsvoltage * tdsvoltage - 255.86 * tdsvoltage * tdsvoltage + 857.39 * tdsvoltage) * 0.5;  
        Blynk.virtualWrite(V0, tds);  
        Serial.print("TDS Value: ");  
        Serial.print(tds);  
        Serial.println(" ppm");  

        // Activate nutrient pump if TDS is below threshold
        if (tds < thresholdTDS) {  
            digitalWrite(npumpPin, HIGH);  
            delay(5000);  
            digitalWrite(npumpPin, LOW);  
        } else {  
            digitalWrite(npumpPin, LOW);  
        }  
    }  
}  

// Function to check and regulate temperature
void checkTemperature() {  
    float humidity = dht.readHumidity();  
    float temperature = dht.readTemperature();  
    if (isnan(humidity) || isnan(temperature)) {  
        Serial.println("Failed to read from DHT sensor!");  
        return;  
    }  
    Blynk.virtualWrite(V2, temperature);  
    Serial.print("%  Temp: ");  
    Serial.print(temperature);  
    Serial.println("°C");  

    // Control fan operation based on temperature thresholds
    if (temperature > tempFanOn) {  
        digitalWrite(fanPin, HIGH);  
    } else if (temperature < tempFanOff) {  
        digitalWrite(fanPin, LOW);  
    } else {  
        if (millis() - lastLoopTime >= 4 * 60 * 60 * 1000) {  
            digitalWrite(fanPin, !digitalRead(fanPin));  
            lastLoopTime = millis();  
        }  
    }  
}  

// Function to check water level using ultrasonic sensor
void checkDistance() {  
    long duration, distance;  
    digitalWrite(ultraSonicTrig, LOW);  
    delayMicroseconds(2);  
    digitalWrite(ultraSonicTrig, HIGH);  
    delayMicroseconds(10);  
    digitalWrite(ultraSonicTrig, LOW);  
    duration = pulseIn(ultraSonicEcho, HIGH);  
    distance = duration * 0.034 / 2;  

    Blynk.virtualWrite(V3, distance);  
    Serial.print("Distance: ");  
    Serial.print(distance);  
    Serial.println(" cm");  

    // Alert if water level is low
    if (distance > maxDistance) {  
        Blynk.logEvent("distance", "Water level low! Distance: " + String(distance) + "cm");  
    }  
    delay(5000);  
}  

// Function to control water pump
void wpump() {  
    if (millis() - previousMillis >= inter) {  
        previousMillis = millis();  
        Serial.println("Turning on pump");  
        digitalWrite(wpumpPin, LOW);  
        delay(20000);  
        Serial.println("Turning off pump");  
        digitalWrite(wpumpPin, HIGH);  
    }  
    delay(500);  
}  

// Function to control air pump
void airpump() {  
    unsigned long curMillis = millis();  
    if (curMillis - preMillis >= interval) {  
        preMillis = curMillis;  
        airpumpState = !airpumpState;  
        digitalWrite(airpumpPin, airpumpState);  
        if (airpumpState) {  
            Serial.println("Air pump is ON");  
        }  
    }  
}