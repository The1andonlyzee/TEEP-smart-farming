#define ESP_SERIAL Serial1
#define DEBUG_SERIAL Serial

String ssid = "K410";
String password = "amaap67674";
String thingsboardHost = "192.168.0.148";
String thingsboardPort = "1883";
String accessToken = "qg6gytglzblm0nn2suq3";  // Replace with your token

// soil sensors
#define SOIL_MOISTURE_PIN A0
#define MSG_DELAY 2000

void setup() {
  DEBUG_SERIAL.begin(115200);
  ESP_SERIAL.begin(115200);
  
  delay(2000);
  DEBUG_SERIAL.println("Starting ESP-01 Setup...");
  
  // Initialize ESP-01
  sendCommand("AT+RST", 2000);
  sendCommand("AT+CWMODE=1", 1000);
  
  // Connect to WiFi
  String connectCmd = "AT+CWJAP=\"" + ssid + "\",\"" + password + "\"";
  sendCommand(connectCmd, 8000);
  
  // Enable single connection mode
  sendCommand("AT+CIPMUX=0", 1000);
  
  DEBUG_SERIAL.println("Setup complete!");
}

void loop() {
  // Read sensor
  int moistureValue = analogRead(SOIL_MOISTURE_PIN);
  DEBUG_SERIAL.print("Moisture: ");
  DEBUG_SERIAL.println(moistureValue);
  
  // Send to ThingsBoard via MQTT
  publishMQTT("v1/devices/me/telemetry", "{\"moisture\":" + String(moistureValue) + "}");
  
  delay(MSG_DELAY); // Send every minute
}

void sendCommand(String cmd, int timeout) {
  ESP_SERIAL.println(cmd);
  long int time = millis();
  
  while((time + timeout) > millis()) {
    while(ESP_SERIAL.available()) {
      char c = ESP_SERIAL.read();
      DEBUG_SERIAL.write(c);
    }
  }
  DEBUG_SERIAL.println();
}

void publishMQTT(String topic, String payload) {
  DEBUG_SERIAL.println("Publishing to MQTT...");
  
  // Connect to MQTT broker
  String startConn = "AT+CIPSTART=\"TCP\",\"" + thingsboardHost + "\"," + thingsboardPort;
  sendCommand(startConn, 3000);
  delay(500);
  
  // Build MQTT CONNECT packet
  String clientId = "ArduinoMega_" + String(millis());
  byte connectPacket[256];
  int connectLen = buildMQTTConnect(connectPacket, clientId, accessToken);
  
  // Send CONNECT packet
  String sendCmd = "AT+CIPSEND=" + String(connectLen);
  ESP_SERIAL.println(sendCmd);
  delay(500);
  
  ESP_SERIAL.write(connectPacket, connectLen);
  delay(1000);
  
  // Wait for CONNACK
  waitForResponse("SEND OK", 2000);
  
  // Build MQTT PUBLISH packet
  byte publishPacket[256];
  int publishLen = buildMQTTPublish(publishPacket, topic, payload);
  
  // Send PUBLISH packet
  sendCmd = "AT+CIPSEND=" + String(publishLen);
  ESP_SERIAL.println(sendCmd);
  delay(500);
  
  ESP_SERIAL.write(publishPacket, publishLen);
  delay(1000);
  
  waitForResponse("SEND OK", 2000);
  
  // Close connection
  delay(500);
  sendCommand("AT+CIPCLOSE", 1000);
  
  DEBUG_SERIAL.println("MQTT publish complete!");
}

int buildMQTTConnect(byte* packet, String clientId, String username) {
  int pos = 0;
  
  // Fixed header
  packet[pos++] = 0x10;  // CONNECT packet type
  
  // Remaining length (calculate later)
  int remainingLenPos = pos++;
  
  // Variable header - Protocol name
  packet[pos++] = 0x00;
  packet[pos++] = 0x04;
  packet[pos++] = 'M';
  packet[pos++] = 'Q';
  packet[pos++] = 'T';
  packet[pos++] = 'T';
  
  // Protocol level (MQTT 3.1.1)
  packet[pos++] = 0x04;
  
  // Connect flags (username, clean session)
  packet[pos++] = 0xC2;  // Username flag + Clean session
  
  // Keep alive (60 seconds)
  packet[pos++] = 0x00;
  packet[pos++] = 0x3C;
  
  // Payload - Client ID
  packet[pos++] = (clientId.length() >> 8) & 0xFF;
  packet[pos++] = clientId.length() & 0xFF;
  for(int i = 0; i < clientId.length(); i++) {
    packet[pos++] = clientId[i];
  }
  
  // Username (access token)
  packet[pos++] = (username.length() >> 8) & 0xFF;
  packet[pos++] = username.length() & 0xFF;
  for(int i = 0; i < username.length(); i++) {
    packet[pos++] = username[i];
  }
  
  // Calculate and set remaining length
  int remainingLen = pos - 2;
  packet[remainingLenPos] = remainingLen;
  
  return pos;
}

int buildMQTTPublish(byte* packet, String topic, String payload) {
  int pos = 0;
  
  // Fixed header
  packet[pos++] = 0x30;  // PUBLISH packet type, QoS 0
  
  // Remaining length (calculate later)
  int remainingLenPos = pos++;
  
  // Variable header - Topic name
  packet[pos++] = (topic.length() >> 8) & 0xFF;
  packet[pos++] = topic.length() & 0xFF;
  for(int i = 0; i < topic.length(); i++) {
    packet[pos++] = topic[i];
  }
  
  // Payload
  for(int i = 0; i < payload.length(); i++) {
    packet[pos++] = payload[i];
  }
  
  // Calculate and set remaining length
  int remainingLen = pos - 2;
  packet[remainingLenPos] = remainingLen;
  
  return pos;
}

bool waitForResponse(String expected, int timeout) {
  String response = "";
  long int time = millis();
  
  while((time + timeout) > millis()) {
    while(ESP_SERIAL.available()) {
      char c = ESP_SERIAL.read();
      response += c;
      DEBUG_SERIAL.write(c);
      
      if(response.indexOf(expected) != -1) {
        return true;
      }
    }
  }
  return false;
}