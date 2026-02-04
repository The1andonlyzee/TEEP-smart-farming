#define ESP_SERIAL Serial1
#define DEBUG_SERIAL Serial

String ssid = "K410";
String password = "amaap67674";
String thingsboardHost = "192.168.0.148";
String thingsboardPort = "1883";
String accessToken = "qg6gytglzblm0nn2suq3";

// soil sensors
#define SOIL_MOISTURE_PIN A0
#define MSG_DELAY 5000

void setup() {
  DEBUG_SERIAL.begin(115200);
  ESP_SERIAL.begin(115200);
  
  delay(2000);
  DEBUG_SERIAL.println("=== ESP-01 MQTT Setup ===");
  
  // Initialize ESP-01
  DEBUG_SERIAL.println("Resetting ESP-01...");
  sendCommand("AT+RST", 2000);
  
  DEBUG_SERIAL.println("Setting station mode...");
  sendCommand("AT+CWMODE=1", 1000);
  
  // Connect to WiFi
  DEBUG_SERIAL.print("Connecting to WiFi: ");
  DEBUG_SERIAL.println(ssid);
  String connectCmd = "AT+CWJAP=\"" + ssid + "\",\"" + password + "\"";
  sendCommand(connectCmd, 8000);
  
  // Enable single connection mode
  DEBUG_SERIAL.println("Setting single connection mode...");
  sendCommand("AT+CIPMUX=0", 1000);
  
  DEBUG_SERIAL.println("=== Setup Complete ===\n");
}

void loop() {
  // Read sensor
  int moistureValue = analogRead(SOIL_MOISTURE_PIN);
  DEBUG_SERIAL.println("\n=== New Reading ===");
  DEBUG_SERIAL.print("Moisture Value: ");
  DEBUG_SERIAL.println(moistureValue);
  
  // Send to ThingsBoard via MQTT
  publishMQTT("v1/devices/me/telemetry", "{\"moisture\":" + String(moistureValue) + "}");
  
  DEBUG_SERIAL.print("Waiting ");
  DEBUG_SERIAL.print(MSG_DELAY/1000);
  DEBUG_SERIAL.println(" seconds...\n");
  delay(MSG_DELAY);
}

void sendCommand(String cmd, int timeout) {
  DEBUG_SERIAL.print(">> ");
  DEBUG_SERIAL.println(cmd);
  
  ESP_SERIAL.println(cmd);
  long int time = millis();
  String response = "";
  
  while((time + timeout) > millis()) {
    while(ESP_SERIAL.available()) {
      char c = ESP_SERIAL.read();
      response += c;
      DEBUG_SERIAL.write(c);
    }
  }
  
  DEBUG_SERIAL.println();
}

void publishMQTT(String topic, String payload) {
  DEBUG_SERIAL.println("--- MQTT Publish Start ---");
  
  // Step 1: Connect to MQTT broker
  DEBUG_SERIAL.print("Connecting to MQTT broker: ");
  DEBUG_SERIAL.print(thingsboardHost);
  DEBUG_SERIAL.print(":");
  DEBUG_SERIAL.println(thingsboardPort);
  
  String startConn = "AT+CIPSTART=\"TCP\",\"" + thingsboardHost + "\"," + thingsboardPort;
  sendCommand(startConn, 3000);
  delay(500);
  
  // Step 2: Build MQTT CONNECT packet
  String clientId = "ArduinoMega_" + String(millis());
  DEBUG_SERIAL.print("Client ID: ");
  DEBUG_SERIAL.println(clientId);
  DEBUG_SERIAL.print("Access Token: ");
  DEBUG_SERIAL.println(accessToken);
  
  byte connectPacket[256];
  int connectLen = buildMQTTConnect(connectPacket, clientId, accessToken);
  
  DEBUG_SERIAL.print("CONNECT packet size: ");
  DEBUG_SERIAL.print(connectLen);
  DEBUG_SERIAL.println(" bytes");
  DEBUG_SERIAL.print("CONNECT packet (hex): ");
  for(int i = 0; i < connectLen; i++) {
    if(connectPacket[i] < 0x10) DEBUG_SERIAL.print("0");
    DEBUG_SERIAL.print(connectPacket[i], HEX);
    DEBUG_SERIAL.print(" ");
  }
  DEBUG_SERIAL.println();
  
  // Step 3: Send CONNECT packet
  DEBUG_SERIAL.println("Sending MQTT CONNECT...");
  String sendCmd = "AT+CIPSEND=" + String(connectLen);
  ESP_SERIAL.println(sendCmd);
  delay(500);
  
  ESP_SERIAL.write(connectPacket, connectLen);
  delay(2000);
  
  // Wait for CONNACK
  DEBUG_SERIAL.println("Waiting for CONNACK...");
  bool connackReceived = waitForMQTTResponse(0x20, 3000); // 0x20 = CONNACK
  if(connackReceived) {
    DEBUG_SERIAL.println("✓ CONNACK received - Connected to MQTT broker!");
  } else {
    DEBUG_SERIAL.println("✗ CONNACK not received - Connection failed!");
    sendCommand("AT+CIPCLOSE", 1000);
    return;
  }
  
  // Step 4: Build MQTT PUBLISH packet
  DEBUG_SERIAL.print("Publishing to topic: ");
  DEBUG_SERIAL.println(topic);
  DEBUG_SERIAL.print("Payload: ");
  DEBUG_SERIAL.println(payload);
  
  byte publishPacket[256];
  int publishLen = buildMQTTPublish(publishPacket, topic, payload);
  
  DEBUG_SERIAL.print("PUBLISH packet size: ");
  DEBUG_SERIAL.print(publishLen);
  DEBUG_SERIAL.println(" bytes");
  DEBUG_SERIAL.print("PUBLISH packet (hex): ");
  for(int i = 0; i < publishLen; i++) {
    if(publishPacket[i] < 0x10) DEBUG_SERIAL.print("0");
    DEBUG_SERIAL.print(publishPacket[i], HEX);
    DEBUG_SERIAL.print(" ");
  }
  DEBUG_SERIAL.println();
  
  // Step 5: Send PUBLISH packet
  DEBUG_SERIAL.println("Sending MQTT PUBLISH...");
  sendCmd = "AT+CIPSEND=" + String(publishLen);
  ESP_SERIAL.println(sendCmd);
  delay(500);
  
  ESP_SERIAL.write(publishPacket, publishLen);
  delay(2000);
  
  bool publishSent = waitForResponse("SEND OK", 2000);
  if(publishSent) {
    DEBUG_SERIAL.println("✓ PUBLISH sent successfully!");
  } else {
    DEBUG_SERIAL.println("✗ PUBLISH failed!");
  }
  
  // Step 6: Close connection
  delay(500);
  DEBUG_SERIAL.println("Closing connection...");
  sendCommand("AT+CIPCLOSE", 1000);
  
  DEBUG_SERIAL.println("--- MQTT Publish Complete ---");
}

int buildMQTTConnect(byte* packet, String clientId, String username) {
  int pos = 0;
  
  // Fixed header
  packet[pos++] = 0x10;  // CONNECT packet type
  
  // Calculate remaining length first
  int remainingLen = 10 + 2 + clientId.length() + 2 + username.length();
  
  // Encode remaining length (variable length encoding)
  pos += encodeRemainingLength(&packet[pos], remainingLen);
  
  // Variable header - Protocol name "MQTT"
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
  
  return pos;
}

int buildMQTTPublish(byte* packet, String topic, String payload) {
  int pos = 0;
  
  // Fixed header
  packet[pos++] = 0x30;  // PUBLISH packet type, QoS 0
  
  // Calculate remaining length
  int remainingLen = 2 + topic.length() + payload.length();
  
  // Encode remaining length
  pos += encodeRemainingLength(&packet[pos], remainingLen);
  
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
  
  return pos;
}

int encodeRemainingLength(byte* buffer, int length) {
  int pos = 0;
  do {
    byte encodedByte = length % 128;
    length = length / 128;
    if (length > 0) {
      encodedByte = encodedByte | 128;
    }
    buffer[pos++] = encodedByte;
  } while (length > 0);
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

bool waitForMQTTResponse(byte expectedType, int timeout) {
  long int time = millis();
  
  while((time + timeout) > millis()) {
    while(ESP_SERIAL.available()) {
      char c = ESP_SERIAL.read();
      DEBUG_SERIAL.write(c);
      
      // Look for MQTT packet type in response
      if((byte)c == expectedType) {
        // Wait a bit more to get full packet
        delay(100);
        while(ESP_SERIAL.available()) {
          DEBUG_SERIAL.write(ESP_SERIAL.read());
        }
        return true;
      }
    }
  }
  return false;
}
