/*
 * OPTION A: NodeMCU V3 Code - WiFi Module for Arduino Mega
 * 
 * This code runs on NodeMCU V3 ESP8266
 * Handles WiFi and MQTT communication
 * Receives sensor data from Arduino Mega via Serial
 * Sends RPC commands back to Arduino Mega
 * 
 * Upload this to NodeMCU V3
 */

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// WiFi credentials
const char* ssid = "K410";
const char* password = "amaap67674";

// ThingsBoard settings
const char* mqttServer = "192.168.0.107";
const int mqttPort = 1883;
const char* accessToken = "qe79zygknyij3wvuy0dz";

WiFiClient espClient;
PubSubClient mqttClient(espClient);

// Data from Arduino Mega
int moisturePercent = 0;
int moisture = 0;
bool relayState = false;
String mode = "auto";

unsigned long lastReconnect = 0;
#define RECONNECT_INTERVAL 5000

void setup() {
  Serial.begin(115200);
  
  Serial.println("\n\n=== NodeMCU V3 WiFi Module ===");
  
  // Connect to WiFi
  connectWiFi();
  
  // Setup MQTT
  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setBufferSize(512);
  
  Serial.println("=== NodeMCU Ready ===");
}

void loop() {
  // Maintain WiFi
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }
  
  // Maintain MQTT
  if (!mqttClient.connected()) {
    unsigned long now = millis();
    if (now - lastReconnect > RECONNECT_INTERVAL) {
      lastReconnect = now;
      reconnectMQTT();
    }
  }
  mqttClient.loop();
  
  // Check for data from Arduino Mega
  if (Serial.available()) {
    String data = Serial.readStringUntil('\n');
    parseDataFromMega(data);
  }
}

void connectWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✓ WiFi Connected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("RSSI: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  } else {
    Serial.println("\n✗ WiFi Failed!");
  }
}

void reconnectMQTT() {
  Serial.print("Connecting to MQTT...");
  
  if (mqttClient.connect("NodeMCU_Client", accessToken, NULL)) {
    Serial.println(" ✓ Connected!");
    
    if (mqttClient.subscribe("v1/devices/me/rpc/request/+")) {
      Serial.println("Subscribed to RPC");
    }
  } else {
    Serial.print(" ✗ Failed! rc=");
    Serial.println(mqttClient.state());
  }
}

void parseDataFromMega(String data) {
  // Format: moisture,relay,mode
  // Example: 723,1,auto
  
  int firstComma = data.indexOf(',');
  int secondComma = data.indexOf(',', firstComma + 1);
  int thirdComma = data.indexOf(',', secondComma + 1);
  
  if (firstComma > 0 && secondComma > 0 && thirdComma> 0) {
    moisture = data.substring(0, firstComma).toInt();
    moisturePercent = data.substring(firstComma + 1, secondComma).toInt();
    relayState = data.substring(secondComma + 1, thirdComma) == "1";
    mode = data.substring(thirdComma + 1);
    mode.trim();
    
    // Send to ThingsBoard
    sendTelemetry();
  }
}

void sendTelemetry() {
  char payload[150];
  snprintf(payload, sizeof(payload), 
           "{\"moisture\":%d,\"moisture_percentage\":%d%,\"relay\":%s,\"mode\":\"%s\"}", 
           moisture,
           moisturePercent,
           relayState ? "true" : "false",
           mode.c_str());
  
  Serial.print("Publishing: ");
  Serial.println(payload);
  
  if (mqttClient.publish("v1/devices/me/telemetry", payload)) {
    Serial.println("✓ Published!");
  } else {
    Serial.println("✗ Publish failed!");
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("RPC received: ");
  
  char message[256];
  if (length < sizeof(message)) {
    memcpy(message, payload, length);
    message[length] = '\0';
    Serial.println(message);
    
    if (strstr(topic, "rpc/request/") != NULL) {
      processRPC(message);
    }
  }
}

void processRPC(char* message) {
  // Parse and send command to Arduino Mega
  
  if (strstr(message, "\"method\":\"setRelay\"") != NULL) {
    if (strstr(message, "\"params\":true") != NULL || strstr(message, "true") != NULL) {
      Serial.println("RELAY_ON");  // Send to Mega
      Serial.println("→ Sent RELAY_ON to Mega");
    } 
    else if (strstr(message, "\"params\":false") != NULL || strstr(message, "false") != NULL) {
      Serial.println("RELAY_OFF");  // Send to Mega
      Serial.println("→ Sent RELAY_OFF to Mega");
    }
  }
  else if (strstr(message, "\"method\":\"setMode\"") != NULL) {
    if (strstr(message, "\"params\":\"auto\"") != NULL || strstr(message, "auto") != NULL) {
      Serial.println("MODE_AUTO");  // Send to Mega
      Serial.println("→ Sent MODE_AUTO to Mega");
    } 
    else if (strstr(message, "\"params\":\"manual\"") != NULL || strstr(message, "manual") != NULL) {
      Serial.println("MODE_MANUAL");  // Send to Mega
      Serial.println("→ Sent MODE_MANUAL to Mega");
    }
  }
}
