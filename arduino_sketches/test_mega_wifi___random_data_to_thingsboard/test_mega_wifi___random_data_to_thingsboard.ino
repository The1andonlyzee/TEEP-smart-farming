#include <WiFiEsp.h>
#include <PubSubClient.h>

// WiFi credentials
char ssid[] = "K410";
char pass[] = "amaap67674";

// ThingsBoard settings
const char* mqtt_server = "192.168.0.148";
const int mqtt_port = 1883;
const char* token = "qg6gytglzblm0nn2suq3";

// Initialize clients
WiFiEspClient espClient;
PubSubClient client(espClient);

// Timing
unsigned long lastMsg = 0;
const long interval = 5000;  // Send data every 5 seconds

void setup() {
  Serial.begin(9600);
  Serial1.begin(115200);  // ESP-12F communication
  // Serial1.begin(9600);  // ESP-12F communication

  Serial.println("\n=== ThingsBoard MQTT Demo ===");
  
  // Initialize ESP module
  WiFi.init(&Serial1);
  
  // Check for ESP module
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("ERROR: WiFi module not present");
    while (true);  // Stop here
  }
  
  Serial.print("ESP firmware version: ");
  Serial.println(WiFi.firmwareVersion());
  
  // Connect to WiFi
  connectWiFi();
  
  // Configure MQTT
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqttCallback);
  
  Serial.println("Setup complete!\n");
}

void loop() {
  // Maintain MQTT connection
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();
  
  // Send data periodically
  unsigned long now = millis();
  if (now - lastMsg > interval) {
    lastMsg = now;
    sendTelemetry();
  }
}

void connectWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  
  int status = WiFi.begin(ssid, pass);
  
  // Wait for connection
  int attempts = 0;
  while (status != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    status = WiFi.status();
    attempts++;
  }
  
  if (status == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    printWiFiStatus();
  } else {
    Serial.println("\nWiFi connection failed!");
  }
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Connecting to ThingsBoard MQTT...");
    
    // Attempt to connect (clientId, username, password)
    if (client.connect("ArduinoMega", token, NULL)) {
      Serial.println("connected!");
      
      // Subscribe to RPC requests (optional)
      client.subscribe("v1/devices/me/rpc/request/+");
      
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" - retrying in 5 seconds");
      delay(5000);
    }
  }
}

void sendTelemetry() {
  // Generate sample sensor data
  float temperature = 20.0 + random(0, 100) / 10.0;
  float humidity = 40.0 + random(0, 400) / 10.0;
  int lightLevel = random(0, 1024);
  
  // Build JSON payload
  String payload = "{";
  payload += "\"temperature\":" + String(temperature, 1) + ",";
  payload += "\"humidity\":" + String(humidity, 1) + ",";
  payload += "\"light\":" + String(lightLevel);
  payload += "}";
  
  Serial.print("Publishing: ");
  Serial.println(payload);
  
  // Publish to ThingsBoard
  if (client.publish("v1/devices/me/telemetry", payload.c_str())) {
    Serial.println("✓ Data sent successfully");
  } else {
    Serial.println("✗ Failed to send data");
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  
  // Handle RPC requests here if needed
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