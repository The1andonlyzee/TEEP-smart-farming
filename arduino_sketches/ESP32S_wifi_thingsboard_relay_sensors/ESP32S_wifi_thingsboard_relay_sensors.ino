#include <WiFi.h>

// MUST be defined BEFORE including PubSubClient
#define MQTT_MAX_PACKET_SIZE 512

#include <PubSubClient.h>
#include <ArduinoJson.h>

// WiFi credentials
const char* ssid = "K410";
const char* password = "amaap67674";

// ThingsBoard server
const char* mqtt_server = "192.168.0.148";  // e.g., "192.168.1.100" or "thingsboard.cloud"
const char* token = "sRUkVthA1BIJWDFxFP3t";


// Pin definitions
#define RELAY_PIN 16

// 5 Soil moisture sensor pins
#define SOIL_SENSOR_1 36
#define SOIL_SENSOR_2 39
#define SOIL_SENSOR_3 34
#define SOIL_SENSOR_4 35
#define SOIL_SENSOR_5 32

const int soilSensorPins[5] = {SOIL_SENSOR_1, SOIL_SENSOR_2, SOIL_SENSOR_3, SOIL_SENSOR_4, SOIL_SENSOR_5};

// MQTT topics
char telemetryTopic[] = "v1/devices/me/telemetry";
char attributesTopic[] = "v1/devices/me/attributes";
char rpcTopic[] = "v1/devices/me/rpc/request/+";

// WiFi and MQTT clients
WiFiClient espClient;
PubSubClient client(espClient);

// Timing variables
unsigned long lastSensorRead = 0;
const long sensorInterval = 5000;

// Sensor variables
int soilMoistureRaw[5] = {0, 0, 0, 0, 0};
int soilMoisturePercent[5] = {0, 0, 0, 0, 0};
bool relayState = false;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n=== ESP32 ThingsBoard Multi-Sensor Setup ===");
  
  // Print MQTT buffer size for verification
  Serial.print("MQTT Max Packet Size: ");
  Serial.println(MQTT_MAX_PACKET_SIZE);
  
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  
  for (int i = 0; i < 5; i++) {
    pinMode(soilSensorPins[i], INPUT);
    Serial.print("Initialized Soil Sensor ");
    Serial.print(i + 1);
    Serial.print(" on GPIO");
    Serial.println(soilSensorPins[i]);
  }
  
  setupWiFi();

  // Set buffer size BEFORE connecting to MQTT
  client.setBufferSize(512);  // ADD THIS LINE
  Serial.print("MQTT Buffer Size set to: ");
  Serial.println(client.getBufferSize());  // Verify buffer size

  client.setServer(mqtt_server, 1883);
  client.setCallback(mqttCallback);
  reconnectMQTT();
  
  Serial.println("=== Setup Complete ===\n");
}

void loop() {
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();
  
  unsigned long currentMillis = millis();
  if (currentMillis - lastSensorRead >= sensorInterval) {
    lastSensorRead = currentMillis;
    
    readAllSensors();
    displaySensorReadings();
    sendTelemetry();
  }
}

void setupWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Signal strength (RSSI): ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  } else {
    Serial.println("\nWiFi connection failed!");
  }
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Connecting to ThingsBoard MQTT...");
    
    if (client.connect("ESP32Client", token, NULL)) {
      Serial.println("connected!");
      client.subscribe(rpcTopic);
      Serial.print("Subscribed to: ");
      Serial.println(rpcTopic);
      sendAttributes();
      
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" retrying in 5 seconds...");
      delay(5000);
    }
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  
  char message[length + 1];
  memcpy(message, payload, length);
  message[length] = '\0';
  Serial.println(message);
  
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, message);
  
  if (error) {
    Serial.print("JSON parse failed: ");
    Serial.println(error.c_str());
    return;
  }
  
  const char* method = doc["method"];
  
  if (strcmp(method, "setRelay") == 0) {
    bool state = doc["params"];
    setRelay(state);
    
    String requestId = topic;
    requestId = requestId.substring(requestId.lastIndexOf('/') + 1);
    sendRpcResponse(requestId, state);
  }
  else if (strcmp(method, "getRelay") == 0) {
    String requestId = topic;
    requestId = requestId.substring(requestId.lastIndexOf('/') + 1);
    sendRpcResponse(requestId, relayState);
  }
}

void setRelay(bool state) {
  relayState = state;
  digitalWrite(RELAY_PIN, state ? HIGH : LOW);
  
  Serial.print("Relay set to: ");
  Serial.println(state ? "ON" : "OFF");
  
  StaticJsonDocument<100> doc;
  doc["relay"] = state;
  
  char buffer[100];
  serializeJson(doc, buffer);
  client.publish(telemetryTopic, buffer);
}

void readAllSensors() {
  for (int i = 0; i < 5; i++) {
    soilMoistureRaw[i] = analogRead(soilSensorPins[i]);
    soilMoisturePercent[i] = map(soilMoistureRaw[i], 4095, 0, 0, 100);
    soilMoisturePercent[i] = constrain(soilMoisturePercent[i], 0, 100);
    delay(10);
  }
}

void displaySensorReadings() {
  Serial.println("\n=== Soil Moisture Readings ===");
  for (int i = 0; i < 5; i++) {
    Serial.print("Sensor ");
    Serial.print(i + 1);
    Serial.print(" (GPIO");
    Serial.print(soilSensorPins[i]);
    Serial.print("): ");
    Serial.print("Raw=");
    Serial.print(soilMoistureRaw[i]);
    Serial.print(" | ");
    Serial.print(soilMoisturePercent[i]);
    Serial.println("%");
  }
  Serial.println("==============================\n");
}

void sendTelemetry() {
  StaticJsonDocument<512> doc;
  
  doc["soilMoisture1"] = soilMoisturePercent[0];
  doc["soilMoisture2"] = soilMoisturePercent[1];
  doc["soilMoisture3"] = soilMoisturePercent[2];
  doc["soilMoisture4"] = soilMoisturePercent[3];
  doc["soilMoisture5"] = soilMoisturePercent[4];
  
  doc["soilMoistureRaw1"] = soilMoistureRaw[0];
  doc["soilMoistureRaw2"] = soilMoistureRaw[1];
  doc["soilMoistureRaw3"] = soilMoistureRaw[2];
  doc["soilMoistureRaw4"] = soilMoistureRaw[3];
  doc["soilMoistureRaw5"] = soilMoistureRaw[4];
  
  doc["relay"] = relayState;
  
  int avgMoisture = 0;
  for (int i = 0; i < 5; i++) {
    avgMoisture += soilMoisturePercent[i];
  }
  avgMoisture = avgMoisture / 5;
  doc["avgMoisture"] = avgMoisture;
  
  char buffer[512];
  size_t len = serializeJson(doc, buffer);
  
  Serial.print("Publishing telemetry (");
  Serial.print(len);
  Serial.print(" bytes): ");
  Serial.println(buffer);
  
  if (client.publish(telemetryTopic, buffer)) {
    Serial.println("✓ Telemetry sent successfully!");
  } else {
    Serial.println("✗ Failed to send telemetry");
    Serial.print("MQTT state: ");
    Serial.println(client.state());
  }
}

void sendAttributes() {
  StaticJsonDocument<300> doc;
  doc["deviceType"] = "ESP32-5Sensors";
  doc["firmwareVersion"] = "1.1";
  doc["ipAddress"] = WiFi.localIP().toString();
  doc["rssi"] = WiFi.RSSI();
  doc["sensorCount"] = 5;
  
  JsonArray sensors = doc.createNestedArray("sensorPins");
  for (int i = 0; i < 5; i++) {
    sensors.add(soilSensorPins[i]);
  }
  
  char buffer[300];
  serializeJson(doc, buffer);
  
  Serial.print("Publishing attributes: ");
  Serial.println(buffer);
  
  client.publish(attributesTopic, buffer);
}

void sendRpcResponse(String requestId, bool value) {
  String responseTopic = "v1/devices/me/rpc/response/" + requestId;
  
  StaticJsonDocument<50> doc;
  doc["result"] = value;
  
  char buffer[50];
  serializeJson(doc, buffer);
  
  Serial.print("Sending RPC response to: ");
  Serial.println(responseTopic);
  Serial.print("Response: ");
  Serial.println(buffer);
  
  client.publish(responseTopic.c_str(), buffer);
}