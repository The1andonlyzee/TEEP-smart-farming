/*
 * Arduino Mega + ESP-01 using WiFiEsp Library
 * 
 * Requirements:
 * 1. Install WiFiEsp library from Library Manager
 * 2. ESP-01 must have AT firmware (you already have this working)
 * 3. PubSubClient library for MQTT
 */

#include <WiFiEsp.h>
#include <PubSubClient.h>

// WiFi credentials
char ssid[] = "K410";
char pass[] = "amaap67674";

// ThingsBoard settings
const char* mqttServer = "192.168.0.148";
const int mqttPort = 1883;
const char* accessToken = "qg6gytglzblm0nn2suq3";

// Soil sensor
#define SOIL_MOISTURE_PIN A0
#define MSG_DELAY 5000

// ESP-01 connection (using Serial1)
#define ESP_SERIAL Serial1

// Initialize WiFi and MQTT clients
WiFiEspClient espClient;
PubSubClient mqttClient(espClient);

unsigned long lastMsg = 0;

void setup() {
  Serial.begin(115200);
  
  // Initialize ESP-01
  ESP_SERIAL.begin(115200);
  
  Serial.println("=== Arduino Mega + ESP-01 with WiFiEsp ===");
  
  // Initialize WiFiEsp library
  WiFi.init(&ESP_SERIAL);
  
  // Check for ESP-01 module
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("ERROR: WiFi module not detected!");
    Serial.println("Check connections and AT firmware");
    while (true);
  }
  
  Serial.print("ESP-01 firmware version: ");
  Serial.println(WiFi.firmwareVersion());
  
  // Connect to WiFi
  connectWiFi();
  
  // Setup MQTT
  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCallback(mqttCallback);
  
  Serial.println("=== Setup Complete ===\n");
}

void loop() {
  // Maintain WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected! Reconnecting...");
    connectWiFi();
  }
  
  // Maintain MQTT connection
  if (!mqttClient.connected()) {
    reconnectMQTT();
  }
  mqttClient.loop();
  
  // Send data every MSG_DELAY
  unsigned long now = millis();
  if (now - lastMsg > MSG_DELAY) {
    lastMsg = now;
    
    // Read sensor
    int moistureValue = analogRead(SOIL_MOISTURE_PIN);
    
    Serial.println("\n=== New Reading ===");
    Serial.print("Moisture: ");
    Serial.println(moistureValue);
    
    // Send to ThingsBoard
    sendTelemetry(moistureValue);
  }
}

void connectWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  
  // Attempt to connect
  int status = WiFi.begin(ssid, pass);
  
  if (status == WL_CONNECTED) {
    Serial.println("✓ Connected to WiFi!");
    printWiFiStatus();
  } else {
    Serial.println("✗ Connection failed!");
  }
}

void reconnectMQTT() {
  // Loop until reconnected
  while (!mqttClient.connected()) {
    Serial.print("Connecting to MQTT broker...");
    
    // Attempt to connect with access token as username
    if (mqttClient.connect("ArduinoMegaClient", accessToken, NULL)) {
      Serial.println(" ✓ Connected!");
    } else {
      Serial.print(" ✗ Failed! rc=");
      Serial.println(mqttClient.state());
      Serial.println("Retrying in 5 seconds...");
      delay(5000);
    }
  }
}

void sendTelemetry(int moisture) {
  // Create JSON payload
  char payload[100];
  snprintf(payload, sizeof(payload), "{\"moisture\":%d}", moisture);
  
  Serial.print("Publishing: ");
  Serial.println(payload);
  
  // Publish to ThingsBoard
  if (mqttClient.publish("v1/devices/me/telemetry", payload)) {
    Serial.println("✓ Published successfully!");
  } else {
    Serial.println("✗ Publish failed!");
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void printWiFiStatus() {
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
  
  long rssi = WiFi.RSSI();
  Serial.print("Signal strength (RSSI): ");
  Serial.print(rssi);
  Serial.println(" dBm");
}
