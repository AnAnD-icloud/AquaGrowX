// Define Blynk credentials for authentication
#define BLYNK_TEMPLATE_ID "your_template_id"  // Unique template identifier
#define BLYNK_TEMPLATE_NAME "your_template_name"  // Template name for your IoT project
#define BLYNK_AUTH_TOKEN "your_auth_token"  // Authentication token for Blynk

// Include necessary libraries for ESP8266 and sensor functionality
#include <ESP8266WiFi.h>  // Wi-Fi connectivity for ESP8266
#include <BlynkSimpleEsp8266.h>  // Blynk library for ESP8266
#include <time.h>  // Time functions
#include <WiFiClient.h>  // Wi-Fi client library
#include <ESP8266WebServer.h>  // Web server for handling client requests
#include <NTPClient.h>  // NTP client to fetch current time
#include <WiFiUdp.h>  // UDP communication for NTP client

// Create instances for Blynk timer, Wi-Fi client, and NTP client
BlynkTimer timer;
WiFiClient espClient;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 19800);  // NTP client with time zone adjustment
ESP8266WebServer server(80);  // Web server running on port 80

// Wi-Fi credentials
char ssid[] = "your_ssid";  // Wi-Fi network SSID
char pass[] = "your_password";  // Wi-Fi password

// Define sensor and actuator pins
const int phPin = A0;  // pH sensor analog pin
const int apumpPin = D4;  // Acid pump control pin
const int bpumpPin = D3;  // Base pump control pin
const int lightPin = D0;  // Light control pin

// Define pH thresholds for pump activation
const int phLowThreshold = 6.5;  // Lower pH threshold
const int phHighThreshold = 7.5;  // Upper pH threshold

// Pump control timing variables
const unsigned long pumpInterval = 300000;  // Interval for pump operation (in milliseconds)
unsigned long previousMillis = 0;

// Function to control automatic light operation based on time
void automaticlight() {
  server.handleClient();  // Handle web server requests
  timeClient.update();  // Update NTP client
  String formattedTime = timeClient.getFormattedTime();  // Fetch formatted time
  Serial.print("Current time: ");
  Serial.println(formattedTime);
  int currentHour = timeClient.getHours();
  int currentMinute = timeClient.getMinutes();

  // Turn light ON at 6 PM
  if (currentHour == 18 && currentMinute == 0) {
    digitalWrite(lightPin, HIGH);
    Serial.println("Light turned ON");
  }

  // Turn light OFF at 7 AM
  if (currentHour == 7 && currentMinute == 0) {
    digitalWrite(lightPin, LOW);
    Serial.println("Light turned OFF");
  }

  delay(1000);
}

// Function to handle root web server request
void handleRoot() {
  server.send(200, "text/plain", "Hello from ESP8266!");
}

// Function to handle unknown web requests
void handleNotFound() {
  server.send(404, "text/plain", "Not found");
}

// Function to manually turn light ON
void turnOnLight() {
  digitalWrite(lightPin, HIGH);
  Serial.println("Light turned ON");
}

// Function to manually turn light OFF
void turnOffLight() {
  digitalWrite(lightPin, LOW);
  Serial.println("Light turned OFF");
}

// Blynk handler to receive input and control light
BLYNK_WRITE(V4) {
  int value = param.asInt();
  if (value == 1) {
    turnOnLight();
  } else {
    turnOffLight();
  }
}

// Function to monitor and regulate pH levels
void checkpH() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= pumpInterval) {
    previousMillis = currentMillis;
    int sensorValue = analogRead(phPin);  // Read pH sensor value
    float voltage = sensorValue * (3.3 / 1023.0);  // Convert ADC reading to voltage
    float slope = (9 - 4) / (2.5 - 1.5);  // Calibration formula for pH sensor
    float intercept = 4 - slope * 1.5;
    float ph = slope * voltage + intercept;  // Calculate pH value
    Blynk.virtualWrite(V1, ph);  // Send pH value to Blynk app
    Serial.print("pH: ");
    Serial.println(ph);
    delay(2000);

    // Adjust pH using pumps based on thresholds
    if (ph < phLowThreshold) {
      digitalWrite(bpumpPin, HIGH);  // Activate base pump
      delay(5000);
      digitalWrite(bpumpPin, LOW);
      digitalWrite(apumpPin, LOW);
    } else if (ph > phHighThreshold) {
      digitalWrite(apumpPin, HIGH);  // Activate acid pump
      delay(5000);
      digitalWrite(apumpPin, LOW);
      digitalWrite(bpumpPin, LOW);
    } else {
      digitalWrite(apumpPin, LOW);
      digitalWrite(bpumpPin, LOW);
    }
  }
}

// Setup function to initialize Wi-Fi, Blynk, server, and pins
void setup() {
  Serial.begin(9600);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  timeClient.begin();  // Start NTP client

  timer.setInterval(60000, automaticlight);  // Set automatic light check interval

  // Setup web server routes
  server.on("/", HTTP_GET, handleRoot);
  server.onNotFound(handleNotFound);
  server.begin();

  // Set pin modes
  pinMode(phPin, INPUT);
  pinMode(apumpPin, OUTPUT);
  pinMode(bpumpPin, OUTPUT);
  pinMode(lightPin, OUTPUT);

  // Ensure pumps are OFF initially
  digitalWrite(apumpPin, LOW);
  digitalWrite(bpumpPin, LOW);
}

// Main loop function
void loop() {
  Blynk.run();  // Run Blynk processes
  timer.run();  // Execute timer-controlled functions
  checkpH();  // Monitor pH
  automaticlight();  // Control lighting
}