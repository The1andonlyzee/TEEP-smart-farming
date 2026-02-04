/*
 * Arduino Mega + ESP-01 using WiFiEsp Library with Relay Control
 * 
 * Requirements:
 * 1. Install WiFiEsp library from Library Manager
 * 2. ESP-01 must have AT firmware (you already have this working)
 * 3. PubSubClient library for MQTT
 * 4. Relay module connected to pin 7
 * 
 * Features:
 * - Reads soil moisture sensor
 * - Sends data to ThingsBoard via MQTT
 * - Controls relay based on moisture threshold
 * - Can be controlled remotely via ThingsBoard
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
#define MOISTURE_THRESHOLD 500  // Adjust this value based on your sensor
                                // Lower value = drier soil
                                // Higher value = wetter soil

// Operating modes
bool autoMode = false;  // true = automatic control, false = manual control
bool manualRelayState = true;  // Manual relay state when in manual mode

// ESP-01 connection (using Serial1)
#define ESP_SERIAL Serial1

// Initialize WiFi and MQTT clients
WiFiEspClient espClient;
PubSubClient mqttClient(espClient);

unsigned long lastMsg = 0;
int currentMoistureValue = 0;
bool relayState = false;

void setup() {
  Serial.begin(115200);
  
  // Initialize relay pin
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);  // Start with relay OFF (HIGH = OFF for most modules)
  
  // Initialize ESP-01
  ESP_SERIAL.begin(115200);
  
  Serial.println("=== Arduino Mega + ESP-01 with WiFiEsp + Relay ===");
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
    currentMoistureValue = analogRead(SOIL_MOISTURE_PIN);
    
    Serial.println("\n=== New Reading ===");
    Serial.print("Moisture: ");
    Serial.println(currentMoistureValue);
    
    // Control relay based on mode
    // if (autoMode) {
    //   controlRelayAuto(currentMoistureValue);
    // } else {
    //   controlRelayManual(manualRelayState);
    // }
    // controlRelayManual(manualRelayState);

    // Send to ThingsBoard
    sendTelemetry(currentMoistureValue, relayState);
  }
}

// void controlRelayAuto(int moisture) {
//   // If moisture is below threshold (soil is dry), turn relay ON
//   if (moisture < MOISTURE_THRESHOLD && !relayState) {
//     relayState = true;
//     digitalWrite(RELAY_PIN, LOW);  // LOW = ON for most relay modules
//     Serial.println("🔴 RELAY ON - Soil is DRY (Auto Mode)");
//   } 
//   // If moisture is above threshold (soil is wet), turn relay OFF
//   else if (moisture >= MOISTURE_THRESHOLD && relayState) {
//     relayState = false;
//     digitalWrite(RELAY_PIN, HIGH);  // HIGH = OFF for most relay modules
//     Serial.println("🔵 RELAY OFF - Soil is WET (Auto Mode)");
//   }
// }

void controlRelayManual(bool state) {
  if (state != relayState) {
    relayState = state;
    digitalWrite(RELAY_PIN, state ? LOW : HIGH);  // LOW = ON, HIGH = OFF
    Serial.print("Relay set to: ");
    Serial.println(state ? "ON (Manual)" : "OFF (Manual)");
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
      
      // Subscribe to RPC commands from ThingsBoard
      mqttClient.subscribe("v1/devices/me/rpc/request/+");
      Serial.println("Subscribed to RPC commands");
      
    } else {
      Serial.print(" ✗ Failed! rc=");
      Serial.println(mqttClient.state());
      Serial.println("Retrying in 5 seconds...");
      delay(5000);
    }
  }
}

void sendTelemetry(int moisture, bool relay) {
  // Create JSON payload with both moisture and relay state
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

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  
  // Print payload
  char message[length + 1];
  for (int i = 0; i < length; i++) {
    message[i] = (char)payload[i];
  }
  message[length] = '\0';
  Serial.println(message);
  
  // Parse RPC commands from ThingsBoard
  // Expected format: {"method":"setRelay","params":true}
  // or: {"method":"setMode","params":"auto"}
  
  if (strstr(topic, "rpc/request/") != NULL) {
    handleRPCCommand(message);
  }
}

void handleRPCCommand(char* message) {
  // Simple parsing for relay control
  if (strstr(message, "\"method\":\"setRelay\"") != NULL) {
    if (strstr(message, "\"params\":true") != NULL) {
      Serial.println("RPC: Setting relay ON");
      autoMode = false;
      manualRelayState = true;
      controlRelayManual(true);
    } else if (strstr(message, "\"params\":false") != NULL) {
      Serial.println("RPC: Setting relay OFF");
      autoMode = false;
      manualRelayState = false;
      controlRelayManual(false);
    }
  }
  else if (strstr(message, "\"method\":\"setMode\"") != NULL) {
    if (strstr(message, "\"params\":\"auto\"") != NULL) {
      Serial.println("RPC: Switching to AUTO mode");
      autoMode = true;
    } else if (strstr(message, "\"params\":\"manual\"") != NULL) {
      Serial.println("RPC: Switching to MANUAL mode");
      autoMode = false;
    }
  }
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
