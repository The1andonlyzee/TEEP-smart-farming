#include <WiFiEsp.h>
#include <PubSubClient.h>

// WiFi credentials
char ssid[] = "K410";
char pass[] = "amaap67674";

// ThingsBoard settings
const char* mqtt_server = "192.168.0.148";
const int mqtt_port = 1883;
const char* token = "sRUkVthA1BIJWDFxFP3t";

// Soil moisture sensor
#define SOIL_MOISTURE_PIN A0
#define LED_PIN 13

// Calibration values (adjust these for your sensor)
const int AIR_VALUE = 1023;    // Sensor value in air (dry)
const int WATER_VALUE = 300;   // Sensor value in water (wet)

// WiFi and MQTT clients
WiFiEspClient espClient;
PubSubClient client(espClient);

// Timing
unsigned long lastMsg = 0;
const long interval = 5000;  // Send data every 5 seconds

void setup() {
  Serial.begin(9600);
  Serial1.begin(115200);
  
  // Increase MQTT packet size (important!)
  client.setBufferSize(512);
  
  pinMode(LED_PIN, OUTPUT);
  pinMode(SOIL_MOISTURE_PIN, INPUT);
  
  Serial.println("\n=== Soil Moisture Monitor ===");
  
  // Initialize WiFi module
  WiFi.init(&Serial1);
  
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("ERROR: WiFi module not found!");
    while (true) {
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      delay(100);
    }
  }
  
  Serial.print("Firmware: ");
  Serial.println(WiFi.firmwareVersion());
  
  // Connect to WiFi
  connectWiFi();
  
  // Setup MQTT
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
  
  // Send telemetry data
  unsigned long now = millis();
  if (now - lastMsg > interval) {
    lastMsg = now;
    sendSoilData();
  }
}

void connectWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, pass);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✓ WiFi connected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n✗ WiFi failed!");
  }
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Connecting to ThingsBoard...");
    
    // Use simple client ID like the working version
    if (client.connect("ArduinoMega", token, NULL)) {
      Serial.println("connected!");
      
      // Blink LED to indicate connection
      for (int i = 0; i < 3; i++) {
        digitalWrite(LED_PIN, HIGH);
        delay(100);
        digitalWrite(LED_PIN, LOW);
        delay(100);
      }
      
    } else {
      Serial.print("failed (rc=");
      Serial.print(client.state());
      Serial.println(") - retrying in 5s...");
      
      // Debug info
      Serial.print("WiFi status: ");
      Serial.println(WiFi.status());
      
      delay(5000);
    }
  }
}

void sendSoilData() {
  // Read raw sensor value
  int sensorValue = analogRead(SOIL_MOISTURE_PIN);
  
  // Convert to moisture percentage
  int moisturePercent = map(sensorValue, AIR_VALUE, WATER_VALUE, 0, 100);
  moisturePercent = constrain(moisturePercent, 0, 100);
  
  // Determine soil condition
  String condition = getSoilCondition(moisturePercent);
  
  // Build JSON payload - keep it compact
  String payload = "{";
  payload += "\"moisture\":" + String(moisturePercent) + ",";
  payload += "\"raw\":" + String(sensorValue) + ",";
  payload += "\"status\":\"" + condition + "\"";
  payload += "}";
  
  // Print to serial
  Serial.println("─────────────────────");
  Serial.print("Raw: ");
  Serial.print(sensorValue);
  Serial.print(" | Moisture: ");
  Serial.print(moisturePercent);
  Serial.print("% | ");
  Serial.println(condition);
  
  // Publish to ThingsBoard
  if (client.publish("v1/devices/me/telemetry", payload.c_str())) {
    Serial.println("✓ Sent to ThingsBoard");
    digitalWrite(LED_PIN, HIGH);
    delay(50);
    digitalWrite(LED_PIN, LOW);
  } else {
    Serial.println("✗ Failed to send");
  }
}

String getSoilCondition(int moisture) {
  if (moisture < 20) return "VeryDry";
  else if (moisture < 40) return "Dry";
  else if (moisture < 60) return "Moist";
  else if (moisture < 80) return "Wet";
  else return "VeryWet";
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("← Message: ");
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}