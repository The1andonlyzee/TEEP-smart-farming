/*
 * OPTION A: Arduino Mega + NodeMCU V3 as WiFi Module
 * 
 * Similar to ESP-01 setup but MUCH easier:
 * - No voltage divider needed (NodeMCU has 5V tolerant RX)
 * - No power issues (has onboard regulator)
 * - More reliable
 * 
 * Arduino Mega: Handles sensors and relay
 * NodeMCU V3: Handles WiFi and MQTT
 * 
 * Communication: Serial between Mega and NodeMCU
 */

// ============================================================================
// ARDUINO MEGA CODE - Upload this to Arduino Mega
// ============================================================================

#define SOIL_MOISTURE_PIN A0
#define RELAY_PIN 7
#define MOISTURE_THRESHOLD 500

bool autoMode = true;
bool manualRelayState = false;
bool relayState = false;
int currentMoistureValueRaw = 0;
int currentMoistureValuePercent = 0;

unsigned long lastSensorRead = 0;
unsigned long lastDataSent = 0;
#define SENSOR_INTERVAL 1000
#define SEND_INTERVAL 5000

void setup() {
  // Serial for debugging
  Serial.begin(115200);
  
  // Serial1 for NodeMCU communication
  Serial1.begin(115200);
  
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);  // Relay OFF
  
  Serial.println("=== Arduino Mega + NodeMCU V3 ===");
  Serial.println("Arduino: Sensors + Relay");
  Serial.println("NodeMCU: WiFi + MQTT");
}

void loop() {
  unsigned long now = millis();
  
  // Read sensor periodically
  if (now - lastSensorRead > SENSOR_INTERVAL) {
    lastSensorRead = now;
    currentMoistureValueRaw = analogRead(SOIL_MOISTURE_PIN);
    currentMoistureValuePercent = map(currentMoistureValueRaw, 700, 250, 0, 100);
    currentMoistureValuePercent = constrain(currentMoistureValuePercent, 0, 100);

    // Auto mode control
    if (autoMode) {
      controlRelayAuto(currentMoistureValueRaw);
    }
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

void controlRelayAuto(int moisture) {
  if (moisture > MOISTURE_THRESHOLD && !relayState) {
    relayState = true;
    digitalWrite(RELAY_PIN, LOW);
    Serial.println("🔴 RELAY ON - Soil DRY (Auto)");
  } 
  else if (moisture <= MOISTURE_THRESHOLD && relayState) {
    relayState = false;
    digitalWrite(RELAY_PIN, HIGH);
    Serial.println("🔵 RELAY OFF - Soil WET (Auto)");
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
  // Send data as simple format: moisture,relay,mode
  Serial1.print(currentMoistureValueRaw);
  Serial1.print(",");
  Serial1.print(currentMoistureValuePercent);
  Serial1.print(",");
  Serial1.print(relayState ? "1" : "0");
  Serial1.print(",");
  Serial1.println(autoMode ? "auto" : "manual");
  
  Serial.print("Sent to NodeMCU: ");
  Serial.print(currentMoistureValueRaw);
  Serial.print(", moisture percentage : ");
  Serial.print(currentMoistureValuePercent);
  Serial.print(", relay=");
  Serial.print(relayState);
  Serial.print(", mode=");
  Serial.println(autoMode ? "auto" : "manual");
}

void processCommand(String cmd) {
  cmd.trim();
  Serial.print("Command from NodeMCU: ");
  Serial.println(cmd);
  
  if (cmd == "RELAY_ON") {
    autoMode = false;
    manualRelayState = true;
    controlRelayManual(true);
  }
  else if (cmd == "RELAY_OFF") {
    autoMode = false;
    manualRelayState = false;
    controlRelayManual(false);
  }
  else if (cmd == "MODE_AUTO") {
    autoMode = true;
    Serial.println("Switched to AUTO mode");
  }
  else if (cmd == "MODE_MANUAL") {
    autoMode = false;
    Serial.println("Switched to MANUAL mode");
  }
}
