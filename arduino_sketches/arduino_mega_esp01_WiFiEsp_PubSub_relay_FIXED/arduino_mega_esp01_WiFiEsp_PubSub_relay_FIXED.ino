/*
 * Arduino Mega + ESP-01 using WiFiEsp Library with Relay Control
 * FIXED VERSION - Handles RPC without WiFi disconnection
 * 
 * Requirements:
 * 1. Install WiFiEsp library from Library Manager
 * 2. ESP-01 must have AT firmware
 * 3. PubSubClient library for MQTT
 * 4. Relay module connected to pin 7
 * 
 * FIX: Increased buffer size and improved callback handling to prevent
 *      WiFi disconnection when receiving RPC commands
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

// Relay control
#define RELAY_PIN 7
#define MOISTURE_THRESHOLD 500

// Operating modes
bool autoMode = false;
bool manualRelayState = false;

// ESP-01 connection (using Serial1)
#define ESP_SERIAL Serial1

// Initialize WiFi and MQTT clients
WiFiEspClient espClient;
PubSubClient mqttClient(espClient);

unsigned long lastMsg = 0;
int currentMoistureValue = 0;
bool relayState = false;

// Buffer for incoming RPC messages
char incomingMessage[256];
bool newRPCCommand = false;

void setup() {
  Serial.begin(115200);
  
  // Initialize relay pin
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);
  
  // Initialize ESP-01
  ESP_SERIAL.begin(115200);
  
  Serial.println("=== Arduino Mega + ESP-01 with WiFiEsp + Relay (FIXED) ===");
  Serial.println("Relay Mode: AUTOMATIC");
  Serial.print("Moisture Threshold: ");
  Serial.println(MOISTURE_THRESHOLD);
  
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
  
  // Setup MQTT with increased buffer size
  mqttClient.setBufferSize(1024);  // Increase buffer to handle RPC messages
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
  
  // Process RPC commands outside of callback
  if (newRPCCommand) {
    processRPCCommand(incomingMessage);
    newRPCCommand = false;
  }
  
  // Send data every MSG_DELAY
  unsigned long now = millis();
  if (now - lastMsg > MSG_DELAY) {
    lastMsg = now;
    
    currentMoistureValue = analogRead(SOIL_MOISTURE_PIN);
    
    Serial.println("\n=== New Reading ===");
    Serial.print("Moisture: ");
    Serial.println(currentMoistureValue);
    
    // Control relay based on mode
    if (autoMode) {
      controlRelayAuto(currentMoistureValue);
    } else {
      controlRelayManual(manualRelayState);
    }
    
    // Send to ThingsBoard
    sendTelemetry(currentMoistureValue, relayState);
  }
}

void controlRelayAuto(int moisture) {
  if (moisture < MOISTURE_THRESHOLD && !relayState) {
    relayState = true;
    digitalWrite(RELAY_PIN, LOW);
    Serial.println("🔴 RELAY ON - Soil is DRY (Auto Mode)");
  } 
  else if (moisture >= MOISTURE_THRESHOLD && relayState) {
    relayState = false;
    digitalWrite(RELAY_PIN, HIGH);
    Serial.println("🔵 RELAY OFF - Soil is WET (Auto Mode)");
  }
}

void controlRelayManual(bool state) {
  if (state != relayState) {
    relayState = state;
    digitalWrite(RELAY_PIN, state ? LOW : HIGH);
    Serial.print("Relay set to: ");
    Serial.println(state ? "ON (Manual)" : "OFF (Manual)");
  }
}

void connectWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  
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
      
      // Subscribe to RPC commands from ThingsBoard
      if (mqttClient.subscribe("v1/devices/me/rpc/request/+")) {
        Serial.println("Subscribed to RPC commands");
      } else {
        Serial.println("Failed to subscribe to RPC!");
      }
      
    } else {
      Serial.print(" ✗ Failed! rc=");
      Serial.println(mqttClient.state());
      Serial.println("Retrying in 5 seconds...");
      delay(5000);
    }
  }
}

void sendTelemetry(int moisture, bool relay) {
  // Create JSON payload
  char payload[150];
  snprintf(payload, sizeof(payload), 
           "{\"moisture\":%d,\"relay\":%s,\"mode\":\"%s\"}", 
           moisture, 
           relay ? "true" : "false",
           autoMode ? "auto" : "manual");
  
  Serial.print("Publishing: ");
  Serial.println(payload);
  
  // Publish to ThingsBoard
  if (mqttClient.publish("v1/devices/me/telemetry", payload)) {
    Serial.println("✓ Published successfully!");
  } else {
    Serial.println("✗ Publish failed!");
  }
}

// FIXED: Simplified callback - just store message, don't process here
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  
  // Copy message to buffer
  if (length < sizeof(incomingMessage) - 1) {
    memcpy(incomingMessage, payload, length);
    incomingMessage[length] = '\0';
    Serial.println(incomingMessage);
    
    // Set flag to process in main loop (not in callback!)
    if (strstr(topic, "rpc/request/") != NULL) {
      newRPCCommand = true;
    }
  } else {
    Serial.println("Message too long!");
  }
}

// Process RPC command in main loop (not in callback)
void processRPCCommand(char* message) {
  Serial.println("Processing RPC command...");
  
  // Simple parsing for relay control
  if (strstr(message, "\"method\":\"setRelay\"") != NULL) {
    if (strstr(message, "\"params\":true") != NULL || 
        strstr(message, "true") != NULL) {
      Serial.println("RPC: Setting relay ON");
      autoMode = false;
      manualRelayState = true;
      controlRelayManual(true);
    } else if (strstr(message, "\"params\":false") != NULL || 
               strstr(message, "false") != NULL) {
      Serial.println("RPC: Setting relay OFF");
      autoMode = false;
      manualRelayState = false;
      controlRelayManual(false);
    }
  }
  else if (strstr(message, "\"method\":\"setMode\"") != NULL) {
    if (strstr(message, "\"params\":\"auto\"") != NULL || 
        strstr(message, "auto") != NULL) {
      Serial.println("RPC: Switching to AUTO mode");
      autoMode = true;
    } else if (strstr(message, "\"params\":\"manual\"") != NULL || 
               strstr(message, "manual") != NULL) {
      Serial.println("RPC: Switching to MANUAL mode");
      autoMode = false;
    }
  }
  
  Serial.println("RPC command processed successfully!");
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
