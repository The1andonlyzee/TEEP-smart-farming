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
  delay(100);
  ESP_SERIAL.begin(115200);  // Communication with ESP32
  
  Serial.println("\n\n");
  Serial.println("=====================================");
  Serial.println("  Arduino Mega + ESP32 Bridge v2.0  ");
  Serial.println("=====================================");
  
  // Initialize relay pin
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);  // Relay OFF initially
  Serial.println("[INIT] Relay pin configured (Pin 7)");
  Serial.println("[INIT] Relay state: OFF (HIGH)");
  
  Serial.println("\n[CONFIG] Settings:");
  Serial.println("  - Mode: AUTOMATIC");
  Serial.print("  - Moisture Threshold: ");
  Serial.println(MOISTURE_THRESHOLD);
  Serial.print("  - Reading Interval: ");
  Serial.print(MSG_DELAY / 1000);
  Serial.println(" seconds");
  Serial.print("  - Soil Sensor Pin: A");
  Serial.println(SOIL_MOISTURE_PIN);
  Serial.print("  - Relay Pin: ");
  Serial.println(RELAY_PIN);
  
  Serial.println("\n[SERIAL] ESP32 Communication:");
  Serial.println("  - Port: Serial1 (TX1=Pin18, RX1=Pin19)");
  Serial.println("  - Baud: 115200");
  Serial.println("  - Protocol: JSON over newline");
  
  Serial.println("\n[STATUS] Waiting for ESP32 connection...");
  Serial.println("       (Waiting for 'ESP32_READY' signal)");
  Serial.println("=====================================\n");
}

void loop() {
  // Check for commands from ESP32
  while (ESP_SERIAL.available()) {
    char c = ESP_SERIAL.read();
    Serial.print("[RX] Received char: '");
    Serial.print(c);
    Serial.print("' (0x");
    Serial.print(c, HEX);
    Serial.println(")");
    
    if (c == '\n') {
      Serial.print("[RX] Complete command received: '");
      Serial.print(incomingCommand);
      Serial.println("'");
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
    
    Serial.println("\n╔════════════════════════════════════╗");
    Serial.println("║      NEW SENSOR READING           ║");
    Serial.println("╚════════════════════════════════════╝");
    
    Serial.print("[SENSOR] Reading analog pin A");
    Serial.print(SOIL_MOISTURE_PIN);
    Serial.println("...");
    
    currentMoistureValue = analogRead(SOIL_MOISTURE_PIN);
    
    Serial.print("[SENSOR] Raw ADC value: ");
    Serial.println(currentMoistureValue);
    Serial.print("[SENSOR] Threshold: ");
    Serial.println(MOISTURE_THRESHOLD);
    Serial.print("[SENSOR] Status: ");
    Serial.println(currentMoistureValue < MOISTURE_THRESHOLD ? "DRY ⚠️" : "WET ✓");
    
    // Control relay based on mode
    Serial.print("[MODE] Current mode: ");
    Serial.println(autoMode ? "AUTOMATIC" : "MANUAL");
    
    if (autoMode) {
      Serial.println("[RELAY] Checking auto control conditions...");
      controlRelayAuto(currentMoistureValue);
    } else {
      Serial.print("[RELAY] Manual state commanded: ");
      Serial.println(manualRelayState ? "ON" : "OFF");
      controlRelayManual(manualRelayState);
    }
    
    // Send telemetry to ESP32
    Serial.println("\n[TX] Preparing to send data to ESP32...");
    sendToESP32(currentMoistureValue, relayState);
    Serial.println("════════════════════════════════════\n");
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
  // Build JSON payload
  String payload = "{\"moisture\":";
  payload += String(moisture);
  payload += ",\"relay\":";
  payload += relay ? "true" : "false";
  payload += ",\"mode\":\"";
  payload += autoMode ? "auto" : "manual";
  payload += "\"}";
  
  Serial.println("[TX] ┌─ Sending to ESP32 ─────────");
  Serial.print("[TX] │ Payload: ");
  Serial.println(payload);
  Serial.print("[TX] │ Length: ");
  Serial.print(payload.length());
  Serial.println(" bytes");
  Serial.println("[TX] │ Port: Serial1");
  
  // Send to ESP32
  ESP_SERIAL.println(payload);
  
  Serial.println("[TX] └─ Sent! Waiting for ESP32 response...");
}

void processCommand(String cmd) {
  cmd.trim();
  if (cmd.length() == 0) {
    Serial.println("[CMD] Empty command received, ignoring");
    return;
  }
  
  Serial.println("\n┌─────────────────────────────────");
  Serial.print("│ [CMD] Processing: '");
  Serial.print(cmd);
  Serial.println("'");
  Serial.print("│ [CMD] Length: ");
  Serial.println(cmd.length());
  
  // Parse RPC commands
  if (cmd.indexOf("setRelay:ON") >= 0) {
    Serial.println("│ [RPC] Command: SET RELAY ON");
    Serial.println("│ [MODE] Switching to MANUAL mode");
    autoMode = false;
    manualRelayState = true;
    controlRelayManual(true);
    Serial.println("│ [RPC] ✓ Relay turned ON");
  }
  else if (cmd.indexOf("setRelay:OFF") >= 0) {
    Serial.println("│ [RPC] Command: SET RELAY OFF");
    Serial.println("│ [MODE] Switching to MANUAL mode");
    autoMode = false;
    manualRelayState = false;
    controlRelayManual(false);
    Serial.println("│ [RPC] ✓ Relay turned OFF");
  }
  else if (cmd.indexOf("setMode:AUTO") >= 0) {
    Serial.println("│ [RPC] Command: SET MODE AUTO");
    autoMode = true;
    Serial.println("│ [MODE] ✓ Switched to AUTOMATIC mode");
  }
  else if (cmd.indexOf("setMode:MANUAL") >= 0) {
    Serial.println("│ [RPC] Command: SET MODE MANUAL");
    autoMode = false;
    Serial.println("│ [MODE] ✓ Switched to MANUAL mode");
  }
  else if (cmd.indexOf("ESP32_READY") >= 0) {
    Serial.println("│ [ESP32] ✓✓✓ ESP32 IS READY! ✓✓✓");
    Serial.println("│ [ESP32] WiFi connected successfully");
  }
  else if (cmd.indexOf("ESP32_MQTT_OK") >= 0) {
    Serial.println("│ [ESP32] ✓✓✓ MQTT CONNECTED! ✓✓✓");
    Serial.println("│ [ESP32] ThingsBoard connection established");
  }
  else if (cmd.indexOf("ESP32_PUBLISHED") >= 0) {
    Serial.println("│ [ESP32] ✓ Data published to ThingsBoard");
  }
  else if (cmd.indexOf("ESP32_PUB_FAIL") >= 0) {
    Serial.println("│ [ESP32] ✗ PUBLISH FAILED!");
  }
  else {
    Serial.print("│ [CMD] ⚠️  Unknown command: '");
    Serial.print(cmd);
    Serial.println("'");
  }
  
  Serial.println("└─────────────────────────────────\n");
}
