#define ESP_SERIAL Serial1
#define DEBUG_SERIAL Serial

String ssid = "K410";
String password = "amaap67674";
String thingsboardHost = "192.168.0.148";
String thingsboardPort = "8080";
String accessToken = "qg6gytglzblm0nn2suq3";
 
// soil sensors
#define SOIL_MOISTURE_PIN A0
#define MSG_DELAY 5000

void setup() {
  DEBUG_SERIAL.begin(115200);
  ESP_SERIAL.begin(115200);
  
  delay(2000);
  DEBUG_SERIAL.println("=== ESP-01 HTTP Setup ===");
  
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
  
  DEBUG_SERIAL.println("=== Setup Complete ===\n");
}

void loop() {
  // Read sensor
  int moistureValue = analogRead(SOIL_MOISTURE_PIN);
  
  DEBUG_SERIAL.println("\n=== New Reading ===");
  DEBUG_SERIAL.print("Moisture Value: ");
  DEBUG_SERIAL.println(moistureValue);
  
  // Send data to ThingsBoard
  sendToThingsBoard(moistureValue);
  
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

void sendToThingsBoard(int moisture) {
  DEBUG_SERIAL.println("--- HTTP POST Start ---");
  
  // Create JSON payload
  String jsonData = "{\"moisture\":" + String(moisture) + "}";
  DEBUG_SERIAL.print("JSON Payload: ");
  DEBUG_SERIAL.println(jsonData);
  DEBUG_SERIAL.print("Payload Size: ");
  DEBUG_SERIAL.print(jsonData.length());
  DEBUG_SERIAL.println(" bytes");
  
  // Start TCP connection to ThingsBoard
  DEBUG_SERIAL.print("Connecting to: ");
  DEBUG_SERIAL.print(thingsboardHost);
  DEBUG_SERIAL.print(":");
  DEBUG_SERIAL.println(thingsboardPort);
  
  String startConn = "AT+CIPSTART=\"TCP\",\"" + thingsboardHost + "\"," + thingsboardPort;
  sendCommand(startConn, 3000);
  
  // Prepare HTTP POST request
  String httpRequest = "POST /api/v1/" + accessToken + "/telemetry HTTP/1.1\r\n";
  httpRequest += "Host: " + thingsboardHost + "\r\n";
  httpRequest += "Content-Type: application/json\r\n";
  httpRequest += "Content-Length: " + String(jsonData.length()) + "\r\n";
  httpRequest += "\r\n";
  httpRequest += jsonData;
  
  DEBUG_SERIAL.println("\nHTTP Request:");
  DEBUG_SERIAL.println("---");
  DEBUG_SERIAL.print(httpRequest);
  DEBUG_SERIAL.println("---");
  DEBUG_SERIAL.print("Total Request Size: ");
  DEBUG_SERIAL.print(httpRequest.length());
  DEBUG_SERIAL.println(" bytes\n");
  
  // Send data length
  DEBUG_SERIAL.println("Sending data...");
  String sendCmd = "AT+CIPSEND=" + String(httpRequest.length());
  ESP_SERIAL.println(sendCmd);
  delay(500);
  
  // Send actual data
  ESP_SERIAL.print(httpRequest);
  delay(2000);
  
  // Check for response
  DEBUG_SERIAL.println("\nServer Response:");
  DEBUG_SERIAL.println("---");
  String response = "";
  long startTime = millis();
  while(millis() - startTime < 3000) {
    while(ESP_SERIAL.available()) {
      char c = ESP_SERIAL.read();
      response += c;
      DEBUG_SERIAL.write(c);
    }
  }
  DEBUG_SERIAL.println("---");
  
  // Check if successful
  if(response.indexOf("HTTP/1.1 200") >= 0) {
    DEBUG_SERIAL.println("\n✓ SUCCESS: Data sent to ThingsBoard!");
  } else if(response.indexOf("SEND OK") >= 0) {
    DEBUG_SERIAL.println("\n✓ Data transmitted (waiting for server confirmation)");
  } else {
    DEBUG_SERIAL.println("\n✗ WARNING: Unexpected response");
  }
  
  // Close connection
  delay(500);
  DEBUG_SERIAL.println("\nClosing connection...");
  sendCommand("AT+CIPCLOSE", 1000);
  
  DEBUG_SERIAL.println("--- HTTP POST Complete ---");
}
