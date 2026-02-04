/*
 * ESP32 - WiFi/MQTT Bridge for Arduino Mega
 * Receives sensor data from Arduino via serial
 * Handles WiFi connection and MQTT communication
 * 
 * Connections:
 * ESP32         Arduino Mega
 * TX (GPIO1)  -> RX1 (Pin 19) - direct connection OK
 * RX (GPIO3)  -> TX1 (Pin 18) - USE VOLTAGE DIVIDER! 5V->3.3V
 * GND         -> GND (CRITICAL!)
 */

#include <WiFi.h>
#include <PubSubClient.h>

// WiFi credentials
const char* ssid = "K410";
const char* password = "amaap67674";

// ThingsBoard settings
const char* mqttServer = "192.168.0.148";
const int mqttPort = 1883;
const char* accessToken = "qg6gytglzblm0nn2suq3";

WiFiClient espClient;
PubSubClient mqttClient(espClient);

// Serial communication with Arduino
#define ARDUINO_SERIAL Serial  // Use Serial for communication with Arduino
String incomingData = "";

void setup() {
  // Initialize serial for Arduino communication
  ARDUINO_SERIAL.begin(115200);
  
  delay(1000);
  ARDUINO_SERIAL.println("\n\n");
  ARDUINO_SERIAL.println("╔════════════════════════════════════════╗");
  ARDUINO_SERIAL.println("║   ESP32 WiFi/MQTT Bridge v2.0         ║");
  ARDUINO_SERIAL.println("╚════════════════════════════════════════╝");
  ARDUINO_SERIAL.println("[ESP32] Booting...");
  ARDUINO_SERIAL.print("[ESP32] Chip Model: ");
  ARDUINO_SERIAL.println(ESP.getChipModel());
  ARDUINO_SERIAL.print("[ESP32] CPU Frequency: ");
  ARDUINO_SERIAL.print(ESP.getCpuFreqMHz());
  ARDUINO_SERIAL.println(" MHz");
  ARDUINO_SERIAL.print("[ESP32] Free Heap: ");
  ARDUINO_SERIAL.print(ESP.getFreeHeap());
  ARDUINO_SERIAL.println(" bytes");
  
  ARDUINO_SERIAL.println("\n[CONFIG] Network Settings:");
  ARDUINO_SERIAL.print("  - WiFi SSID: ");
  ARDUINO_SERIAL.println(ssid);
  ARDUINO_SERIAL.print("  - MQTT Server: ");
  ARDUINO_SERIAL.println(mqttServer);
  ARDUINO_SERIAL.print("  - MQTT Port: ");
  ARDUINO_SERIAL.println(mqttPort);
  ARDUINO_SERIAL.print("  - Access Token: ");
  ARDUINO_SERIAL.println(accessToken);
  
  ARDUINO_SERIAL.println("\n[INIT] Starting WiFi connection...");
  // Connect to WiFi
  connectWiFi();
  
  ARDUINO_SERIAL.println("\n[INIT] Configuring MQTT client...");
  // Setup MQTT
  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setBufferSize(1024);
  ARDUINO_SERIAL.println("[MQTT] Buffer size: 1024 bytes");
  ARDUINO_SERIAL.print("[MQTT] Server: ");
  ARDUINO_SERIAL.print(mqttServer);
  ARDUINO_SERIAL.print(":");
  ARDUINO_SERIAL.println(mqttPort);
  
  ARDUINO_SERIAL.println("\n╔════════════════════════════════════════╗");
  ARDUINO_SERIAL.println("║   ESP32 READY - Waiting for data      ║");
  ARDUINO_SERIAL.println("╚════════════════════════════════════════╝\n");
  ARDUINO_SERIAL.println("ESP32_READY");  // Tell Arduino we're ready
}

void loop() {
  static unsigned long lastStatusPrint = 0;
  unsigned long now = millis();
  
  // Print status every 30 seconds
  if (now - lastStatusPrint > 30000) {
    lastStatusPrint = now;
    ARDUINO_SERIAL.println("\n[STATUS] ──────────────────────────────");
    ARDUINO_SERIAL.print("[STATUS] WiFi: ");
    ARDUINO_SERIAL.println(WiFi.status() == WL_CONNECTED ? "✓ Connected" : "✗ Disconnected");
    ARDUINO_SERIAL.print("[STATUS] MQTT: ");
    ARDUINO_SERIAL.println(mqttClient.connected() ? "✓ Connected" : "✗ Disconnected");
    ARDUINO_SERIAL.print("[STATUS] Uptime: ");
    ARDUINO_SERIAL.print(millis() / 1000);
    ARDUINO_SERIAL.println(" seconds");
    ARDUINO_SERIAL.print("[STATUS] Free Heap: ");
    ARDUINO_SERIAL.print(ESP.getFreeHeap());
    ARDUINO_SERIAL.println(" bytes");
    ARDUINO_SERIAL.println("[STATUS] ──────────────────────────────\n");
  }
  
  // Maintain WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    ARDUINO_SERIAL.println("\n[WIFI] ⚠️  CONNECTION LOST!");
    ARDUINO_SERIAL.println("[WIFI] Attempting reconnection...");
    connectWiFi();
  }
  
  // Maintain MQTT connection
  if (!mqttClient.connected()) {
    ARDUINO_SERIAL.println("\n[MQTT] ⚠️  CONNECTION LOST!");
    ARDUINO_SERIAL.println("[MQTT] Attempting reconnection...");
    reconnectMQTT();
  }
  
  // Process MQTT messages
  mqttClient.loop();
  
  // Read data from Arduino
  while (ARDUINO_SERIAL.available()) {
    char c = ARDUINO_SERIAL.read();
    if (c == '\n') {
      if (incomingData.length() > 0) {
        ARDUINO_SERIAL.print("[RX] Complete message from Arduino: '");
        ARDUINO_SERIAL.print(incomingData);
        ARDUINO_SERIAL.println("'");
        ARDUINO_SERIAL.print("[RX] Message length: ");
        ARDUINO_SERIAL.print(incomingData.length());
        ARDUINO_SERIAL.println(" bytes");
        processArduinoData(incomingData);
        incomingData = "";
      }
    } else {
      incomingData += c;
    }
  }
}

void connectWiFi() {
  ARDUINO_SERIAL.println("\n┌─ WiFi Connection Attempt ──────────");
  ARDUINO_SERIAL.print("│ [WIFI] Target SSID: ");
  ARDUINO_SERIAL.println(ssid);
  ARDUINO_SERIAL.print("│ [WIFI] Password length: ");
  ARDUINO_SERIAL.print(strlen(password));
  ARDUINO_SERIAL.println(" chars");
  
  ARDUINO_SERIAL.println("│ [WIFI] Disconnecting any existing connection...");
  WiFi.disconnect(true);
  delay(100);
  
  ARDUINO_SERIAL.println("│ [WIFI] Starting connection...");
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  ARDUINO_SERIAL.print("│ [WIFI] Connecting");
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    ARDUINO_SERIAL.print(".");
    attempts++;
    
    if (attempts % 10 == 0) {
      ARDUINO_SERIAL.println();
      ARDUINO_SERIAL.print("│ [WIFI] Attempt ");
      ARDUINO_SERIAL.print(attempts);
      ARDUINO_SERIAL.print("/30: Status code = ");
      ARDUINO_SERIAL.println(WiFi.status());
    }
  }
  ARDUINO_SERIAL.println();
  
  if (WiFi.status() == WL_CONNECTED) {
    ARDUINO_SERIAL.println("│ [WIFI] ✓✓✓ CONNECTION SUCCESSFUL! ✓✓✓");
    ARDUINO_SERIAL.print("│ [WIFI] IP Address: ");
    ARDUINO_SERIAL.println(WiFi.localIP());
    ARDUINO_SERIAL.print("│ [WIFI] Gateway: ");
    ARDUINO_SERIAL.println(WiFi.gatewayIP());
    ARDUINO_SERIAL.print("│ [WIFI] Subnet: ");
    ARDUINO_SERIAL.println(WiFi.subnetMask());
    ARDUINO_SERIAL.print("│ [WIFI] DNS: ");
    ARDUINO_SERIAL.println(WiFi.dnsIP());
    ARDUINO_SERIAL.print("│ [WIFI] Signal Strength (RSSI): ");
    ARDUINO_SERIAL.print(WiFi.RSSI());
    ARDUINO_SERIAL.println(" dBm");
    ARDUINO_SERIAL.print("│ [WIFI] MAC Address: ");
    ARDUINO_SERIAL.println(WiFi.macAddress());
  } else {
    ARDUINO_SERIAL.println("│ [WIFI] ✗✗✗ CONNECTION FAILED! ✗✗✗");
    ARDUINO_SERIAL.print("│ [WIFI] Final status code: ");
    ARDUINO_SERIAL.println(WiFi.status());
    ARDUINO_SERIAL.println("│ [WIFI] Status meanings:");
    ARDUINO_SERIAL.println("│        0 = WL_IDLE_STATUS");
    ARDUINO_SERIAL.println("│        1 = WL_NO_SSID_AVAIL");
    ARDUINO_SERIAL.println("│        3 = WL_CONNECTED");
    ARDUINO_SERIAL.println("│        4 = WL_CONNECT_FAILED");
    ARDUINO_SERIAL.println("│        6 = WL_DISCONNECTED");
  }
  ARDUINO_SERIAL.println("└────────────────────────────────────\n");
}

void reconnectMQTT() {
  int attempts = 0;
  while (!mqttClient.connected() && attempts < 5) {
    attempts++;
    ARDUINO_SERIAL.println("\n┌─ MQTT Connection Attempt ──────────");
    ARDUINO_SERIAL.print("│ [MQTT] Attempt #");
    ARDUINO_SERIAL.println(attempts);
    ARDUINO_SERIAL.print("│ [MQTT] Server: ");
    ARDUINO_SERIAL.print(mqttServer);
    ARDUINO_SERIAL.print(":");
    ARDUINO_SERIAL.println(mqttPort);
    ARDUINO_SERIAL.print("│ [MQTT] Client ID: ESP32Client");
    ARDUINO_SERIAL.println();
    ARDUINO_SERIAL.print("│ [MQTT] Username (token): ");
    ARDUINO_SERIAL.println(accessToken);
    ARDUINO_SERIAL.println("│ [MQTT] Password: NULL");
    
    ARDUINO_SERIAL.println("│ [MQTT] Calling connect()...");
    bool connected = mqttClient.connect("ESP32Client", accessToken, NULL);
    
    if (connected) {
      ARDUINO_SERIAL.println("│ [MQTT] ✓✓✓ CONNECTION SUCCESSFUL! ✓✓✓");
      ARDUINO_SERIAL.println("│ [MQTT] State: CONNECTED");
      ARDUINO_SERIAL.println("│");
      ARDUINO_SERIAL.println("│ [MQTT] Subscribing to RPC topic...");
      ARDUINO_SERIAL.println("│ [MQTT] Topic: v1/devices/me/rpc/request/+");
      
      if (mqttClient.subscribe("v1/devices/me/rpc/request/+")) {
        ARDUINO_SERIAL.println("│ [MQTT] ✓ Subscribed to RPC commands successfully!");
        ARDUINO_SERIAL.println("ESP32_MQTT_OK");  // Signal to Arduino
      } else {
        ARDUINO_SERIAL.println("│ [MQTT] ✗ Failed to subscribe to RPC!");
        ARDUINO_SERIAL.print("│ [MQTT] State after subscribe: ");
        ARDUINO_SERIAL.println(mqttClient.state());
      }
      
    } else {
      ARDUINO_SERIAL.println("│ [MQTT] ✗✗✗ CONNECTION FAILED! ✗✗✗");
      ARDUINO_SERIAL.print("│ [MQTT] Error code: ");
      int state = mqttClient.state();
      ARDUINO_SERIAL.println(state);
      ARDUINO_SERIAL.println("│ [MQTT] Error meanings:");
      ARDUINO_SERIAL.println("│        -4 = MQTT_CONNECTION_TIMEOUT");
      ARDUINO_SERIAL.println("│        -3 = MQTT_CONNECTION_LOST");
      ARDUINO_SERIAL.println("│        -2 = MQTT_CONNECT_FAILED");
      ARDUINO_SERIAL.println("│        -1 = MQTT_DISCONNECTED");
      ARDUINO_SERIAL.println("│         0 = MQTT_CONNECTED");
      ARDUINO_SERIAL.println("│         1 = MQTT_CONNECT_BAD_PROTOCOL");
      ARDUINO_SERIAL.println("│         2 = MQTT_CONNECT_BAD_CLIENT_ID");
      ARDUINO_SERIAL.println("│         3 = MQTT_CONNECT_UNAVAILABLE");
      ARDUINO_SERIAL.println("│         4 = MQTT_CONNECT_BAD_CREDENTIALS");
      ARDUINO_SERIAL.println("│         5 = MQTT_CONNECT_UNAUTHORIZED");
      
      ARDUINO_SERIAL.println("│");
      ARDUINO_SERIAL.println("│ [MQTT] Troubleshooting:");
      ARDUINO_SERIAL.print("│        - WiFi connected? ");
      ARDUINO_SERIAL.println(WiFi.status() == WL_CONNECTED ? "YES" : "NO");
      ARDUINO_SERIAL.print("│        - Can ping server? Testing...");
      
      // Try to ping the server
      WiFiClient testClient;
      if (testClient.connect(mqttServer, mqttPort)) {
        ARDUINO_SERIAL.println(" YES");
        testClient.stop();
      } else {
        ARDUINO_SERIAL.println(" NO - Server unreachable!");
      }
      
      ARDUINO_SERIAL.println("│");
      ARDUINO_SERIAL.println("│ [MQTT] Retrying in 5 seconds...");
      delay(5000);
    }
    ARDUINO_SERIAL.println("└────────────────────────────────────\n");
  }
  
  if (!mqttClient.connected()) {
    ARDUINO_SERIAL.println("[MQTT] ⚠️  Failed after 5 attempts. Giving up for now.");
  }
}

void processArduinoData(String data) {
  data.trim();
  if (data.length() == 0) {
    ARDUINO_SERIAL.println("[RX] Empty data received, ignoring");
    return;
  }
  
  ARDUINO_SERIAL.println("\n╔════════════════════════════════════════╗");
  ARDUINO_SERIAL.println("║   PROCESSING ARDUINO DATA             ║");
  ARDUINO_SERIAL.println("╚════════════════════════════════════════╝");
  ARDUINO_SERIAL.print("[DATA] Raw: ");
  ARDUINO_SERIAL.println(data);
  ARDUINO_SERIAL.print("[DATA] Length: ");
  ARDUINO_SERIAL.print(data.length());
  ARDUINO_SERIAL.println(" bytes");
  
  // Check if it's JSON telemetry data
  if (data.startsWith("{") && data.endsWith("}")) {
    ARDUINO_SERIAL.println("[DATA] ✓ Valid JSON format detected");
    ARDUINO_SERIAL.println("[DATA] Type: Telemetry");
    
    // Validate JSON structure
    bool hasMoisture = data.indexOf("\"moisture\"") >= 0;
    bool hasRelay = data.indexOf("\"relay\"") >= 0;
    bool hasMode = data.indexOf("\"mode\"") >= 0;
    
    ARDUINO_SERIAL.println("\n[VALIDATE] JSON Structure Check:");
    ARDUINO_SERIAL.print("           - moisture field: ");
    ARDUINO_SERIAL.println(hasMoisture ? "✓" : "✗ MISSING!");
    ARDUINO_SERIAL.print("           - relay field: ");
    ARDUINO_SERIAL.println(hasRelay ? "✓" : "✗ MISSING!");
    ARDUINO_SERIAL.print("           - mode field: ");
    ARDUINO_SERIAL.println(hasMode ? "✓" : "✗ MISSING!");
    
    // Check MQTT connection before publishing
    ARDUINO_SERIAL.println("\n[MQTT] Pre-publish checks:");
    ARDUINO_SERIAL.print("       - MQTT connected? ");
    if (!mqttClient.connected()) {
      ARDUINO_SERIAL.println("✗ NO - Cannot publish!");
      ARDUINO_SERIAL.println("[MQTT] Attempting to reconnect...");
      reconnectMQTT();
      if (!mqttClient.connected()) {
        ARDUINO_SERIAL.println("[MQTT] ✗ Reconnection failed. Data will be lost.");
        ARDUINO_SERIAL.println("ESP32_PUB_FAIL");
        return;
      }
    } else {
      ARDUINO_SERIAL.println("✓ YES");
    }
    
    ARDUINO_SERIAL.print("       - WiFi connected? ");
    ARDUINO_SERIAL.println(WiFi.status() == WL_CONNECTED ? "✓ YES" : "✗ NO");
    
    // Publish to ThingsBoard
    ARDUINO_SERIAL.println("\n[PUBLISH] Attempting to send to ThingsBoard...");
    ARDUINO_SERIAL.println("[PUBLISH] Topic: v1/devices/me/telemetry");
    ARDUINO_SERIAL.print("[PUBLISH] Payload: ");
    ARDUINO_SERIAL.println(data);
    ARDUINO_SERIAL.print("[PUBLISH] Size: ");
    ARDUINO_SERIAL.print(data.length());
    ARDUINO_SERIAL.println(" bytes");
    
    bool publishResult = mqttClient.publish("v1/devices/me/telemetry", data.c_str());
    
    if (publishResult) {
      ARDUINO_SERIAL.println("[PUBLISH] ✓✓✓ SUCCESS! ✓✓✓");
      ARDUINO_SERIAL.println("[PUBLISH] Data sent to ThingsBoard successfully");
      ARDUINO_SERIAL.println("ESP32_PUBLISHED");  // Signal to Arduino
    } else {
      ARDUINO_SERIAL.println("[PUBLISH] ✗✗✗ FAILED! ✗✗✗");
      ARDUINO_SERIAL.print("[PUBLISH] MQTT state after failure: ");
      ARDUINO_SERIAL.println(mqttClient.state());
      ARDUINO_SERIAL.println("[PUBLISH] Possible reasons:");
      ARDUINO_SERIAL.println("           - Buffer overflow (payload too large)");
      ARDUINO_SERIAL.println("           - Connection lost during publish");
      ARDUINO_SERIAL.println("           - Server rejected the publish");
      ARDUINO_SERIAL.println("ESP32_PUB_FAIL");  // Signal to Arduino
    }
  } else {
    ARDUINO_SERIAL.println("[DATA] ✗ Not valid JSON format");
    ARDUINO_SERIAL.println("[DATA] Expected format: {\"moisture\":xxx,\"relay\":true/false,\"mode\":\"xxx\"}");
  }
  
  ARDUINO_SERIAL.println("════════════════════════════════════════\n");
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  ARDUINO_SERIAL.println("\n╔════════════════════════════════════════╗");
  ARDUINO_SERIAL.println("║   MQTT MESSAGE RECEIVED               ║");
  ARDUINO_SERIAL.println("╚════════════════════════════════════════╝");
  ARDUINO_SERIAL.print("[MQTT-RX] Topic: ");
  ARDUINO_SERIAL.println(topic);
  ARDUINO_SERIAL.print("[MQTT-RX] Payload length: ");
  ARDUINO_SERIAL.print(length);
  ARDUINO_SERIAL.println(" bytes");
  
  // Convert payload to string
  String message = "";
  ARDUINO_SERIAL.print("[MQTT-RX] Raw payload: ");
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
    ARDUINO_SERIAL.print((char)payload[i]);
  }
  ARDUINO_SERIAL.println();
  
  ARDUINO_SERIAL.print("[MQTT-RX] Parsed string: ");
  ARDUINO_SERIAL.println(message);
  
  // Check if it's an RPC command
  if (strstr(topic, "rpc/request/") != NULL) {
    ARDUINO_SERIAL.println("[MQTT-RX] ✓ This is an RPC command!");
    ARDUINO_SERIAL.println("[RPC] Parsing and forwarding to Arduino...");
    parseAndForwardRPC(message);
  } else {
    ARDUINO_SERIAL.println("[MQTT-RX] This is NOT an RPC command (ignored)");
    ARDUINO_SERIAL.print("[MQTT-RX] Expected 'rpc/request/' in topic, got: ");
    ARDUINO_SERIAL.println(topic);
  }
  
  ARDUINO_SERIAL.println("════════════════════════════════════════\n");
}

void parseAndForwardRPC(String message) {
  ARDUINO_SERIAL.println("\n┌─ RPC COMMAND PARSER ───────────────");
  ARDUINO_SERIAL.print("│ [RPC] Input: ");
  ARDUINO_SERIAL.println(message);
  ARDUINO_SERIAL.print("│ [RPC] Length: ");
  ARDUINO_SERIAL.println(message.length());
  
  bool commandFound = false;
  
  // Check for setRelay command
  if (message.indexOf("\"method\":\"setRelay\"") >= 0) {
    ARDUINO_SERIAL.println("│ [RPC] Method detected: setRelay");
    
    if (message.indexOf("\"params\":true") >= 0 || message.indexOf("true") >= 0) {
      ARDUINO_SERIAL.println("│ [RPC] Parameter: true (ON)");
      ARDUINO_SERIAL.println("│ [TX] Sending to Arduino: setRelay:ON");
      ARDUINO_SERIAL.println("setRelay:ON");
      commandFound = true;
    } 
    else if (message.indexOf("\"params\":false") >= 0 || message.indexOf("false") >= 0) {
      ARDUINO_SERIAL.println("│ [RPC] Parameter: false (OFF)");
      ARDUINO_SERIAL.println("│ [TX] Sending to Arduino: setRelay:OFF");
      ARDUINO_SERIAL.println("setRelay:OFF");
      commandFound = true;
    } else {
      ARDUINO_SERIAL.println("│ [RPC] ⚠️  Could not parse relay parameter!");
      ARDUINO_SERIAL.println("│ [RPC] Expected 'true' or 'false' in params");
    }
  }
  // Check for setMode command
  else if (message.indexOf("\"method\":\"setMode\"") >= 0) {
    ARDUINO_SERIAL.println("│ [RPC] Method detected: setMode");
    
    if (message.indexOf("\"params\":\"auto\"") >= 0 || message.indexOf("auto") >= 0) {
      ARDUINO_SERIAL.println("│ [RPC] Parameter: auto");
      ARDUINO_SERIAL.println("│ [TX] Sending to Arduino: setMode:AUTO");
      ARDUINO_SERIAL.println("setMode:AUTO");
      commandFound = true;
    }
    else if (message.indexOf("\"params\":\"manual\"") >= 0 || message.indexOf("manual") >= 0) {
      ARDUINO_SERIAL.println("│ [RPC] Parameter: manual");
      ARDUINO_SERIAL.println("│ [TX] Sending to Arduino: setMode:MANUAL");
      ARDUINO_SERIAL.println("setMode:MANUAL");
      commandFound = true;
    } else {
      ARDUINO_SERIAL.println("│ [RPC] ⚠️  Could not parse mode parameter!");
      ARDUINO_SERIAL.println("│ [RPC] Expected 'auto' or 'manual' in params");
    }
  }
  else {
    ARDUINO_SERIAL.println("│ [RPC] ⚠️  Unknown method or malformed command!");
    ARDUINO_SERIAL.println("│ [RPC] Expected methods: 'setRelay' or 'setMode'");
  }
  
  if (commandFound) {
    ARDUINO_SERIAL.println("│ [RPC] ✓ Command parsed and forwarded successfully");
  } else {
    ARDUINO_SERIAL.println("│ [RPC] ✗ Failed to parse command");
  }
  
  ARDUINO_SERIAL.println("└────────────────────────────────────\n");
}
