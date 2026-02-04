/*
 * ESP32 - WiFi/MQTT Bridge for Arduino Mega
 * Receives sensor data from Arduino via serial
 * Handles WiFi connection and MQTT communication
 * 
 * Connections:
 * ESP32         Arduino Mega
 * TX (GPIO1)  -> RX1 (Pin 19) - direct connection OK
 * RX (GPIO3)  -> TX1 (Pin 18) - USE VOLTAGE DIVIDER! 5V->3.3V
 * GND         -> GND (CRITICAL!)
 */

#include <WiFi.h>
#include <PubSubClient.h>

// WiFi credentials
const char* ssid = "K410";
const char* password = "amaap67674";

// ThingsBoard settings
const char* mqttServer = "192.168.0.148";
const int mqttPort = 1883;
const char* accessToken = "qg6gytglzblm0nn2suq3";

WiFiClient espClient;
PubSubClient mqttClient(espClient);

// Serial communication with Arduino
#define ARDUINO_SERIAL Serial  // Use Serial for communication with Arduino
String incomingData = "";

void setup() {
  // Initialize serial for Arduino communication
  ARDUINO_SERIAL.begin(115200);
  
  delay(1000);
  ARDUINO_SERIAL.println("\n=== ESP32 WiFi Bridge Starting ===");
  
  // Connect to WiFi
  connectWiFi();
  
  // Setup MQTT
  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setBufferSize(1024);
  
  ARDUINO_SERIAL.println("=== ESP32 Setup Complete ===");
  ARDUINO_SERIAL.println("ESP32_READY");  // Tell Arduino we're ready
}

void loop() {
  // Maintain WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    ARDUINO_SERIAL.println("WiFi disconnected! Reconnecting...");
    connectWiFi();
  }
  
  // Maintain MQTT connection
  if (!mqttClient.connected()) {
    reconnectMQTT();
  }
  mqttClient.loop();
  
  // Read data from Arduino
  while (ARDUINO_SERIAL.available()) {
    char c = ARDUINO_SERIAL.read();
    if (c == '\n') {
      processArduinoData(incomingData);
      incomingData = "";
    } else {
      incomingData += c;
    }
  }
}

void connectWiFi() {
  ARDUINO_SERIAL.print("Connecting to WiFi: ");
  ARDUINO_SERIAL.println(ssid);
  
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    ARDUINO_SERIAL.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    ARDUINO_SERIAL.println("\n✓ Connected to WiFi!");
    ARDUINO_SERIAL.print("IP Address: ");
    ARDUINO_SERIAL.println(WiFi.localIP());
    ARDUINO_SERIAL.print("Signal Strength: ");
    ARDUINO_SERIAL.print(WiFi.RSSI());
    ARDUINO_SERIAL.println(" dBm");
  } else {
    ARDUINO_SERIAL.println("\n✗ WiFi connection failed!");
  }
}

void reconnectMQTT() {
  while (!mqttClient.connected()) {
    ARDUINO_SERIAL.print("Connecting to MQTT broker...");
    
    if (mqttClient.connect("ESP32Client", accessToken, NULL)) {
      ARDUINO_SERIAL.println(" ✓ Connected!");
      
      // Subscribe to RPC commands
      if (mqttClient.subscribe("v1/devices/me/rpc/request/+")) {
        ARDUINO_SERIAL.println("Subscribed to RPC commands");
      } else {
        ARDUINO_SERIAL.println("Failed to subscribe to RPC!");
      }
      
    } else {
      ARDUINO_SERIAL.print(" ✗ Failed! rc=");
      ARDUINO_SERIAL.println(mqttClient.state());
      ARDUINO_SERIAL.println("Retrying in 5 seconds...");
      delay(5000);
    }
  }
}

void processArduinoData(String data) {
  data.trim();
  if (data.length() == 0) return;
  
  // Check if it's JSON telemetry data
  if (data.startsWith("{") && data.endsWith("}")) {
    ARDUINO_SERIAL.print("Received telemetry: ");
    ARDUINO_SERIAL.println(data);
    
    // Publish to ThingsBoard
    if (mqttClient.publish("v1/devices/me/telemetry", data.c_str())) {
      ARDUINO_SERIAL.println("✓ Published to ThingsBoard");
    } else {
      ARDUINO_SERIAL.println("✗ Publish failed!");
    }
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  ARDUINO_SERIAL.print("MQTT message [");
  ARDUINO_SERIAL.print(topic);
  ARDUINO_SERIAL.print("]: ");
  
  // Convert payload to string
  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  ARDUINO_SERIAL.println(message);
  
  // Parse RPC commands and forward to Arduino
  if (strstr(topic, "rpc/request/") != NULL) {
    parseAndForwardRPC(message);
  }
}

void parseAndForwardRPC(String message) {
  // Parse JSON RPC commands and send simple commands to Arduino
  
  if (message.indexOf("\"method\":\"setRelay\"") >= 0) {
    if (message.indexOf("\"params\":true") >= 0 || message.indexOf("true") >= 0) {
      ARDUINO_SERIAL.println("setRelay:ON");
    } 
    else if (message.indexOf("\"params\":false") >= 0 || message.indexOf("false") >= 0) {
      ARDUINO_SERIAL.println("setRelay:OFF");
    }
  }
  else if (message.indexOf("\"method\":\"setMode\"") >= 0) {
    if (message.indexOf("\"params\":\"auto\"") >= 0 || message.indexOf("auto") >= 0) {
      ARDUINO_SERIAL.println("setMode:AUTO");
    }
    else if (message.indexOf("\"params\":\"manual\"") >= 0 || message.indexOf("manual") >= 0) {
      ARDUINO_SERIAL.println("setMode:MANUAL");
    }
  }
}
