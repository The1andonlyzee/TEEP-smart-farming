/*
 * OPTION A: NodeMCU V3 Code - WiFi Module for Arduino Mega (5 SENSORS)
 * 
 * This code runs on NodeMCU V3 ESP8266
 * Handles WiFi and MQTT communication for 5 sensors
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
const char* accessToken = "6hr4kyzxnd3n7b7291lw"; 

WiFiClient espClient;
PubSubClient mqttClient(espClient);

#define NUM_SENSORS 5

// Data from Arduino Mega for each sensor
int moistureRaw[NUM_SENSORS] = {0};
int moisturePercent[NUM_SENSORS] = {0};
bool relayState = false;
String modes[NUM_SENSORS] = {"auto", "auto", "auto", "auto", "auto"};

unsigned long lastReconnect = 0;
#define RECONNECT_INTERVAL 5000

void setup() {
  Serial.begin(115200);
  
  Serial.println("\n\n=== NodeMCU V3 WiFi Module (5 Sensors) ===");
  
  // Connect to WiFi
  connectWiFi();
  
  // Setup MQTT with larger buffer for 5 sensors
  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setBufferSize(1024);  // Increased for 5 sensors
  
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
  
  if (mqttClient.connect("NodeMCU_5Sensors", accessToken, NULL)) {
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
  // Format: raw1,pct1,relay1,mode1|raw2,pct2,relay2,mode2|...
  // Example: 723,45,1,auto|650,32,0,manual|500,60,1,auto|...
  
  int sensorIndex = 0;
  int startPos = 0;
  
  while (sensorIndex < NUM_SENSORS) {
    int pipePos = data.indexOf('|', startPos);
    String sensorData;
    
    if (pipePos > 0) {
      sensorData = data.substring(startPos, pipePos);
    } else {
      sensorData = data.substring(startPos);
    }
    
    // Parse individual sensor data: raw,pct,relay,mode
    int comma1 = sensorData.indexOf(',');
    int comma2 = sensorData.indexOf(',', comma1 + 1);
    int comma3 = sensorData.indexOf(',', comma2 + 1);
    
// In the parsing section, only read relay state from first sensor:
    if (comma1 > 0 && comma2 > 0 && comma3 > 0) {
      moistureRaw[sensorIndex] = sensorData.substring(0, comma1).toInt();
      moisturePercent[sensorIndex] = sensorData.substring(comma1 + 1, comma2).toInt();
      
      // Only update relay state from first sensor
      if (sensorIndex == 0) {
        relayState = sensorData.substring(comma2 + 1, comma3) == "1";
      }
      
      modes[sensorIndex] = sensorData.substring(comma3 + 1);
      modes[sensorIndex].trim();
    }
    
    sensorIndex++;
    if (pipePos > 0) {
      startPos = pipePos + 1;
    } else {
      break;
    }
  }
  
  // Send to ThingsBoard
  sendTelemetry();
}

void sendTelemetry() {
  // Build JSON with all 5 sensors
  // Format: {"sensor1_raw":723,"sensor1_pct":45,"sensor1_relay":true,"sensor1_mode":"auto",...}
  
  char payload[800];  // Large enough for 5 sensors
  int len = 0;
  
  // Send single relay state for all sensors:
  len += snprintf(payload + len, sizeof(payload) - len, "{");

  for (int i = 0; i < NUM_SENSORS; i++) {
    if (i > 0) {
      len += snprintf(payload + len, sizeof(payload) - len, ",");
    }
    
    len += snprintf(payload + len, sizeof(payload) - len, 
                  "\"sensor%d_raw\":%d,\"sensor%d_pct\":%d,\"sensor%d_mode\":\"%s\"",
                  i + 1, moistureRaw[i],
                  i + 1, moisturePercent[i],
                  i + 1, modes[i].c_str());
  }

  // Add single relay state at the end
  len += snprintf(payload + len, sizeof(payload) - len, 
                ",\"relay\":%s}", 
                relayState ? "true" : "false");
  
  Serial.print("Publishing: ");
  Serial.println(payload);
  
  if (mqttClient.publish("v1/devices/me/telemetry", payload)) {
    Serial.println("✓ Published!");
  } else {
    Serial.println("✗ Publish failed!");
    Serial.print("Payload size: ");
    Serial.println(len);
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("RPC received: ");
  
  char message[512];
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
  // Simplified - no sensor number needed for relay commands
  
  if (strstr(message, "\"method\":\"setRelay\"") != NULL) {
    if (strstr(message, "\"state\":true") != NULL || strstr(message, "true") != NULL) {
      Serial.println("RELAY_ON");
      Serial.println("→ Sent RELAY_ON to Mega");
    } 
    else if (strstr(message, "\"state\":false") != NULL || strstr(message, "false") != NULL) {
      Serial.println("RELAY_OFF");
      Serial.println("→ Sent RELAY_OFF to Mega");
    }
  }
  // Keep mode commands per-sensor (they still make sense for auto/manual decision)
  else if (strstr(message, "\"method\":\"setMode\"") != NULL) {
    char* sensorStr = strstr(message, "\"sensor\":");
    int sensorNum = 0;
    
    if (sensorStr != NULL) {
      sensorNum = atoi(sensorStr + 9);
    }
    
    if (sensorNum >= 1 && sensorNum <= NUM_SENSORS) {
      if (strstr(message, "\"mode\":\"auto\"") != NULL) {
        char cmd[20];
        snprintf(cmd, sizeof(cmd), "MODE_AUTO_%d", sensorNum);
        Serial.println(cmd);
      } 
      else if (strstr(message, "\"mode\":\"manual\"") != NULL) {
        char cmd[20];
        snprintf(cmd, sizeof(cmd), "MODE_MANUAL_%d", sensorNum);
        Serial.println(cmd);
      }
    }
  }
}
