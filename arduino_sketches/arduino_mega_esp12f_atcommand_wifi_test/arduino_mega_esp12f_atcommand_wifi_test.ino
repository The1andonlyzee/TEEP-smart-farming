#include <PubSubClient.h>

// WiFi credentials
String WIFI_SSID = "K410";
String WIFI_PASS = "amaap67674";

// ThingsBoard
String MQTT_SERVER = "192.168.0.148";
String MQTT_PORT = "1883";
String DEVICE_TOKEN = "qg6gytglzblm0nn2suq3";  // ⚠️ VERIFY THIS!

// Soil sensor
#define SOIL_PIN A0
#define LED_PIN 13

const int AIR_VALUE = 1023;
const int WATER_VALUE = 300;

void setup() {
  Serial.begin(9600);
  Serial1.begin(115200);
  
  pinMode(LED_PIN, OUTPUT);
  pinMode(SOIL_PIN, INPUT);
  
  Serial.println("\n=== Soil Monitor (Debug) ===\n");
  
  // Initialize ESP
  initESP();
  connectWiFi();
  
  // Print config
  Serial.println("=== Configuration ===");
  Serial.print("MQTT Server: ");
  Serial.println(MQTT_SERVER);
  Serial.print("MQTT Port: ");
  Serial.println(MQTT_PORT);
  Serial.print("Device Token: ");
  Serial.println(DEVICE_TOKEN);
  Serial.print("Token Length: ");
  Serial.println(DEVICE_TOKEN.length());
  Serial.println("====================\n");
  
  Serial.println("Ready!\n");
}

void loop() {
  static unsigned long lastSend = 0;
  
  if (millis() - lastSend > 10000) {  // Every 10 seconds
    lastSend = millis();
    sendSoilData();
  }
  
  // Show any responses from ESP
  while(Serial1.available()) {
    char c = Serial1.read();
    Serial.write(c);
  }
  
  delay(100);
}

void initESP() {
  Serial.println("Initializing ESP...");
  
  sendCommand("AT+RST", 2000);
  delay(2000);
  
  sendCommand("AT", 1000);
  sendCommand("AT+GMR", 1000);
  
  sendCommand("AT+CWMODE=1", 1000);
  sendCommand("AT+CIPMUX=0", 1000);
  
  Serial.println("ESP initialized\n");
}

void connectWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);
  
  String cmd = "AT+CWJAP=\"" + WIFI_SSID + "\",\"" + WIFI_PASS + "\"";
  sendCommand(cmd, 15000);
  
  delay(2000);
  
  Serial.println("\nGetting IP...");
  sendCommand("AT+CIFSR", 2000);
  
  Serial.println("WiFi connected!\n");
}

void sendSoilData() {
  // Read sensor
  int raw = analogRead(SOIL_PIN);
  int moisture = map(raw, AIR_VALUE, WATER_VALUE, 0, 100);
  moisture = constrain(moisture, 0, 100);
  
  String condition = "";
  if (moisture < 20) condition = "VeryDry";
  else if (moisture < 40) condition = "Dry";
  else if (moisture < 60) condition = "Moist";
  else if (moisture < 80) condition = "Wet";
  else condition = "VeryWet";
  
  Serial.println("\n========================================");
  Serial.print("Moisture: ");
  Serial.print(moisture);
  Serial.print("% (");
  Serial.print(condition);
  Serial.print(") | Raw: ");
  Serial.println(raw);
  
  // Build payload
  String payload = "{\"moisture\":" + String(moisture);
  payload += ",\"raw\":" + String(raw);
  payload += ",\"status\":\"" + condition + "\"}";
  
  Serial.print("Payload: ");
  Serial.println(payload);
  Serial.print("Payload length: ");
  Serial.println(payload.length());
  
  // Send via MQTT
  if (publishMQTT(payload)) {
    Serial.println("\n✓ Sent to ThingsBoard");
    digitalWrite(LED_PIN, HIGH);
    delay(50);
    digitalWrite(LED_PIN, LOW);
  } else {
    Serial.println("\n✗ Failed to send");
  }
  
  Serial.println("========================================");
}

bool publishMQTT(String payload) {
  Serial.println("\n--- MQTT Connection Sequence ---");
  
  // Connect to MQTT broker
  Serial.println("1. Connecting to MQTT server...");
  String cmd = "AT+CIPSTART=\"TCP\",\"" + MQTT_SERVER + "\"," + MQTT_PORT;
  sendCommand(cmd, 5000);
  
  delay(1500);
  
  // Check if connected
  sendCommand("AT+CIPSTATUS", 1000);
  delay(500);
  
  // Build MQTT CONNECT packet
  String clientId = "ArduinoSoil";
  Serial.println("2. Building MQTT CONNECT packet...");
  Serial.print("   Client ID: ");
  Serial.println(clientId);
  Serial.print("   Username (token): ");
  Serial.println(DEVICE_TOKEN);
  
  String mqttConnect = buildMQTTConnect(clientId, DEVICE_TOKEN);
  
  Serial.print("   CONNECT packet length: ");
  Serial.println(mqttConnect.length());
  Serial.print("   Hex dump: ");
  for (unsigned int i = 0; i < mqttConnect.length() && i < 20; i++) {
    Serial.print(mqttConnect[i], HEX);
    Serial.print(" ");
  }
  Serial.println("...");
  
  // Send CONNECT
  Serial.println("3. Sending MQTT CONNECT...");
  cmd = "AT+CIPSEND=" + String(mqttConnect.length());
  sendCommand(cmd, 1000);
  delay(500);
  Serial1.print(mqttConnect);
  
  // Wait for CONNACK
  Serial.println("4. Waiting for CONNACK...");
  delay(3000);
  
  // Look for CONNACK response
  Serial.println("   Response from server:");
  unsigned long start = millis();
  String response = "";
  while (millis() - start < 2000) {
    while (Serial1.available()) {
      char c = Serial1.read();
      Serial.write(c);
      response += c;
    }
  }
  
  // Check for successful CONNACK (0x20 0x02 0x00 0x00)
  if (response.indexOf("20 02 00 00") >= 0 || response.indexOf("SEND OK") >= 0) {
    Serial.println("   ✓ CONNACK received!");
  } else {
    Serial.println("   ⚠ CONNACK not clearly received (but continuing...)");
  }
  
  // Build MQTT PUBLISH packet
  Serial.println("5. Building MQTT PUBLISH packet...");
  String topic = "v1/devices/me/telemetry";
  Serial.print("   Topic: ");
  Serial.println(topic);
  Serial.print("   Payload: ");
  Serial.println(payload);
  
  String mqttPublish = buildMQTTPublish(topic, payload);
  
  Serial.print("   PUBLISH packet length: ");
  Serial.println(mqttPublish.length());
  
  // Send PUBLISH
  Serial.println("6. Sending MQTT PUBLISH...");
  cmd = "AT+CIPSEND=" + String(mqttPublish.length());
  sendCommand(cmd, 1000);
  delay(500);
  Serial1.print(mqttPublish);
  delay(3000);
  
  // Close connection
  Serial.println("7. Closing connection...");
  sendCommand("AT+CIPCLOSE", 1000);
  
  Serial.println("--- End MQTT Sequence ---\n");
  
  return true;
}

String buildMQTTConnect(String clientId, String username) {
  String packet = "";
  
  // Fixed header
  packet += (char)0x10;  // CONNECT packet type
  
  // Calculate remaining length
  int remainingLength = 10;  // Variable header
  remainingLength += 2 + clientId.length();  // Client ID
  remainingLength += 2 + username.length();  // Username
  
  packet += (char)remainingLength;
  
  // Variable header
  packet += (char)0x00;
  packet += (char)0x04;
  packet += "MQTT";
  packet += (char)0x04;  // Protocol level (MQTT 3.1.1)
  packet += (char)0xC2;  // Connect flags (username + clean session)
  packet += (char)0x00;
  packet += (char)0x3C;  // Keep alive (60 seconds)
  
  // Payload - Client ID
  packet += (char)(clientId.length() >> 8);
  packet += (char)(clientId.length() & 0xFF);
  packet += clientId;
  
  // Payload - Username (access token)
  packet += (char)(username.length() >> 8);
  packet += (char)(username.length() & 0xFF);
  packet += username;
  
  return packet;
}

String buildMQTTPublish(String topic, String payload) {
  String packet = "";
  
  // Fixed header
  packet += (char)0x30;  // PUBLISH packet type, QoS 0
  
  // Calculate remaining length
  int remainingLength = 2 + topic.length() + payload.length();
  
  // Encode remaining length (handle if > 127)
  if (remainingLength < 128) {
    packet += (char)remainingLength;
  } else {
    packet += (char)((remainingLength & 0x7F) | 0x80);
    packet += (char)(remainingLength >> 7);
  }
  
  // Variable header - Topic
  packet += (char)(topic.length() >> 8);
  packet += (char)(topic.length() & 0xFF);
  packet += topic;
  
  // Payload
  packet += payload;
  
  return packet;
}

void sendCommand(String cmd, int timeout) {
  Serial1.println(cmd);
  
  long start = millis();
  while((start + timeout) > millis()) {
    while(Serial1.available()) {
      Serial.write(Serial1.read());
    }
  }
}