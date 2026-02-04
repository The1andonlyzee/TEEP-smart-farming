/*
 * Arduino Mega - Serial Bridge to ESP32
 * Handles all sensors/relays, communicates with ESP32 for WiFi
 * 
 * Serial Protocol:
 * - Arduino -> ESP32: JSON data packets
 * - ESP32 -> Arduino: RPC commands
 * 
 * Connections:
 * Arduino Mega   ESP32
 * TX1 (Pin 18) -> RX (via voltage divider 5V->3.3V)
 * RX1 (Pin 19) -> TX (3.3V is safe for Arduino)
 * GND          -> GND (CRITICAL!)
 */

// Soil sensor
#define SOIL_MOISTURE_PIN A0
#define MSG_DELAY 5000

// Relay control
#define RELAY_PIN 7
#define MOISTURE_THRESHOLD 500

// Operating modes
bool autoMode = true;  // Start in auto mode
bool manualRelayState = false;

// ESP32 connection (using Serial1: TX1=pin18, RX1=pin19)
#define ESP_SERIAL Serial1

unsigned long lastMsg = 0;
int currentMoistureValue = 0;
bool relayState = false;

// Buffer for incoming commands from ESP32
String incomingCommand = "";

void setup() {
  Serial.begin(115200);  // Debug output
  ESP_SERIAL.begin(115200);  // Communication with ESP32
  
  // Initialize relay pin
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);  // Relay OFF initially
  
  Serial.println("=== Arduino Mega + ESP32 Bridge ===");
  Serial.println("Mode: AUTOMATIC");
  Serial.print("Threshold: ");
  Serial.println(MOISTURE_THRESHOLD);
  Serial.println("Waiting for ESP32 connection...\n");
}

void loop() {
  // Check for commands from ESP32
  while (ESP_SERIAL.available()) {
    char c = ESP_SERIAL.read();
    if (c == '\n') {
      processCommand(incomingCommand);
      incomingCommand = "";
    } else {
      incomingCommand += c;
    }
  }
  
  // Send data every MSG_DELAY
  unsigned long now = millis();
  if (now - lastMsg > MSG_DELAY) {
    lastMsg = now;
    
    currentMoistureValue = analogRead(SOIL_MOISTURE_PIN);
    
    Serial.println("\n=== New Reading ===");
    Serial.print("Moisture: ");
    Serial.println(currentMoistureValue);
    
    // Control relay based on mode
    if (autoMode) {
      controlRelayAuto(currentMoistureValue);
    } else {
      controlRelayManual(manualRelayState);
    }
    
    // Send telemetry to ESP32
    sendToESP32(currentMoistureValue, relayState);
  }
}

void controlRelayAuto(int moisture) {
  if (moisture < MOISTURE_THRESHOLD && !relayState) {
    relayState = true;
    digitalWrite(RELAY_PIN, LOW);
    Serial.println("🔴 RELAY ON - Soil is DRY (Auto Mode)");
  } 
  else if (moisture >= MOISTURE_THRESHOLD && relayState) {
    relayState = false;
    digitalWrite(RELAY_PIN, HIGH);
    Serial.println("🔵 RELAY OFF - Soil is WET (Auto Mode)");
  }
}

void controlRelayManual(bool state) {
  if (state != relayState) {
    relayState = state;
    digitalWrite(RELAY_PIN, state ? LOW : HIGH);
    Serial.print("Relay set to: ");
    Serial.println(state ? "ON (Manual)" : "OFF (Manual)");
  }
}

void sendToESP32(int moisture, bool relay) {
  // Send JSON to ESP32 via serial
  ESP_SERIAL.print("{\"moisture\":");
  ESP_SERIAL.print(moisture);
  ESP_SERIAL.print(",\"relay\":");
  ESP_SERIAL.print(relay ? "true" : "false");
  ESP_SERIAL.print(",\"mode\":\"");
  ESP_SERIAL.print(autoMode ? "auto" : "manual");
  ESP_SERIAL.println("\"}");
  
  Serial.println("✓ Data sent to ESP32");
}

void processCommand(String cmd) {
  cmd.trim();
  if (cmd.length() == 0) return;
  
  Serial.print("Command from ESP32: ");
  Serial.println(cmd);
  
  // Parse RPC commands
  if (cmd.indexOf("setRelay:ON") >= 0) {
    Serial.println("RPC: Setting relay ON");
    autoMode = false;
    manualRelayState = true;
    controlRelayManual(true);
  }
  else if (cmd.indexOf("setRelay:OFF") >= 0) {
    Serial.println("RPC: Setting relay OFF");
    autoMode = false;
    manualRelayState = false;
    controlRelayManual(false);
  }
  else if (cmd.indexOf("setMode:AUTO") >= 0) {
    Serial.println("RPC: Switching to AUTO mode");
    autoMode = true;
  }
  else if (cmd.indexOf("setMode:MANUAL") >= 0) {
    Serial.println("RPC: Switching to MANUAL mode");
    autoMode = false;
  }
  else if (cmd.indexOf("ESP32_READY") >= 0) {
    Serial.println("✓ ESP32 WiFi connected!");
  }
  else {
    Serial.print("Unknown command: ");
    Serial.println(cmd);
  }
}
