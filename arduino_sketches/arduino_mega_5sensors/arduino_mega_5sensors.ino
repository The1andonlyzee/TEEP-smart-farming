/*
 * OPTION A: Arduino Mega + NodeMCU V3 as WiFi Module - 5 SENSORS VERSION
 * 
 * Arduino Mega: Handles 5 sensors and 5 relays
 * NodeMCU V3: Handles WiFi and MQTT
 * 
 * Communication: Serial1 between Mega and NodeMCU
 * 
 * Hardware Connections:
 * - Soil Sensors: A0, A1, A2, A3, A4
 * - Relays: Pins 7, 8, 9, 10, 11
 * - Serial1 (TX1/RX1) to NodeMCU
 */

// ============================================================================
// ARDUINO MEGA CODE - Upload this to Arduino Mega
// ============================================================================

// Pin definitions
#define SENSOR_1_PIN A0
#define SENSOR_2_PIN A1
#define SENSOR_3_PIN A2
#define SENSOR_4_PIN A3
#define SENSOR_5_PIN A4

#define RELAY_PIN 7

#define MOISTURE_THRESHOLD 500
#define NUM_SENSORS 5

// Sensor arrays
int sensorPins[NUM_SENSORS] = {SENSOR_1_PIN, SENSOR_2_PIN, SENSOR_3_PIN, SENSOR_4_PIN, SENSOR_5_PIN};

// State tracking
int moistureRaw[NUM_SENSORS] = {0};
int moisturePercent[NUM_SENSORS] = {0};
bool relayState = false;
bool autoMode[NUM_SENSORS] = {true, true, true, true, true};

unsigned long lastSensorRead = 0;
unsigned long lastDataSent = 0;
#define SENSOR_INTERVAL 1000
#define SEND_INTERVAL 5000

void setup() {
  // Serial for debugging
  Serial.begin(115200);
  
  // Serial1 for NodeMCU communication
  Serial1.begin(115200);
  
  // Initialize relays (HIGH = OFF for active-low relays)
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);  // Relay OFF
  
  Serial.println("=== Arduino Mega + NodeMCU V3 (5 Sensors) ===");
  Serial.println("Arduino: 5 Sensors + 5 Relays");
  Serial.println("NodeMCU: WiFi + MQTT");
  Serial.println("Pins - Sensors: A0-A4, Relays: 7-11");
}

void loop() {
  unsigned long now = millis();
  
  // Read all sensors periodically
  if (now - lastSensorRead > SENSOR_INTERVAL) {
    lastSensorRead = now;
    readAllSensors();
    
    // Auto mode control for each sensor
    controlRelayAuto();
  }
  
  // Send data to NodeMCU periodically
  if (now - lastDataSent > SEND_INTERVAL) {
    lastDataSent = now;
    sendDataToNodeMCU();
  }
  
  // Check for commands from NodeMCU
  if (Serial1.available()) {
    String cmd = Serial1.readStringUntil('\n');
    processCommand(cmd);
  }
}

void readAllSensors() {
  for (int i = 0; i < NUM_SENSORS; i++) {
    moistureRaw[i] = analogRead(sensorPins[i]);
    // Map 700 (dry) to 0%, 250 (wet) to 100%
    moisturePercent[i] = map(moistureRaw[i], 700, 250, 0, 100);
    moisturePercent[i] = constrain(moisturePercent[i], 0, 100);
  }
}

void controlRelayAuto() {
  int totalMoisture = 0;
  int activeCount = 0;
  
  for (int i = 0; i < NUM_SENSORS; i++) {
    if (autoMode[i]) {
      totalMoisture += moistureRaw[i];
      activeCount++;
    }
  }
  
  if (activeCount > 0) {
    int avgMoisture = totalMoisture / activeCount;
    
    if (avgMoisture > MOISTURE_THRESHOLD && !relayState) {
      relayState = true;
      digitalWrite(RELAY_PIN, LOW);
      Serial.print("🔴 RELAY ON - Avg moisture: ");
      Serial.println(avgMoisture);
    } else if (avgMoisture <= MOISTURE_THRESHOLD && relayState) {
      relayState = false;
      digitalWrite(RELAY_PIN, HIGH);
      Serial.println("🔵 RELAY OFF");
    }
  }
}

void controlRelayManual(bool state) {
  if (state != relayState) {
    relayState = state;
    digitalWrite(RELAY_PIN, state ? LOW : HIGH);
    Serial.print("Relay: ");
    Serial.println(state ? "ON (Manual)" : "OFF (Manual)");
  }
}

void sendDataToNodeMCU() {
  // Format: raw1,pct1,relay1,mode1|raw2,pct2,relay2,mode2|...
  // Example: 723,45,1,auto|650,32,0,manual|...
  
  for (int i = 0; i < NUM_SENSORS; i++) {
    Serial1.print(moistureRaw[i]);
    Serial1.print(",");
    Serial1.print(moisturePercent[i]);
    Serial1.print(",");
    Serial1.print((i == 0) ? (relayState ? "1" : "0") : "0");  // Only send relay on first sensor    
    Serial1.print(",");
    Serial1.print(autoMode[i] ? "auto" : "manual");
    
    if (i < NUM_SENSORS - 1) {
      Serial1.print("|");  // Separator between sensors
    }
  }
  Serial1.println();
  
  // Debug output
  Serial.print("Sent to NodeMCU: ");
  for (int i = 0; i < NUM_SENSORS; i++) {
    Serial.print("S");
    Serial.print(i + 1);
    Serial.print("(");
    Serial.print(moistureRaw[i]);
    Serial.print(",");
    Serial.print(moisturePercent[i]);
    Serial.print("%,R=");
    Serial.print(relayState);
    Serial.print(",");
    Serial.print(autoMode[i] ? "A" : "M");
    Serial.print(") ");
  }
  Serial.println();
}

void processCommand(String cmd) {
  cmd.trim();
  Serial.print("Command from NodeMCU: ");
  Serial.println(cmd);
  
  if (cmd == "RELAY_ON") {
    controlRelayManual(true);
  }
  else if (cmd == "RELAY_OFF") {
    controlRelayManual(false);
  }
  else if (cmd.startsWith("MODE_AUTO_")) {
    int sensor = cmd.substring(10).toInt() - 1;
    if (sensor >= 0 && sensor < NUM_SENSORS) {
      autoMode[sensor] = true;
      Serial.print("Sensor ");
      Serial.print(sensor + 1);
      Serial.println(" switched to AUTO mode");
    }
  }
  else if (cmd.startsWith("MODE_MANUAL_")) {
    int sensor = cmd.substring(12).toInt() - 1;
    if (sensor >= 0 && sensor < NUM_SENSORS) {
      autoMode[sensor] = false;
      Serial.print("Sensor ");
      Serial.print(sensor + 1);
      Serial.println(" switched to MANUAL mode");
    }
  }
}
