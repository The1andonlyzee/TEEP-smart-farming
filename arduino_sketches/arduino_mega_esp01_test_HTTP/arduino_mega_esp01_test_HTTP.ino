#define ESP_SERIAL Serial1
#define DEBUG_SERIAL Serial

String ssid = "K410";
String password = "amaap67674";
String thingsboardHost = "192.168.0.148";
String thingsboardPort = "8080";
String accessToken = "qg6gytglzblm0nn2suq3";
 
// soil sensors
#define SOIL_MOISTURE_PIN A0
#define MSG_DELAY 2000

void setup() {
  DEBUG_SERIAL.begin(115200);
  ESP_SERIAL.begin(115200);
  
  delay(2000);
  
  // Initialize ESP-01
  sendCommand("AT+RST", 2000);
  sendCommand("AT+CWMODE=1", 1000);
  
  // Connect to WiFi
  String connectCmd = "AT+CWJAP=\"" + ssid + "\",\"" + password + "\"";
  sendCommand(connectCmd, 5000);
  
  DEBUG_SERIAL.println("WiFi Connected!");
}

void loop() {
  // Read sensor
  int moistureValue = analogRead(SOIL_MOISTURE_PIN);
  
  // Send data to ThingsBoard
  sendToThingsBoard(moistureValue);
  
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
}

void sendToThingsBoard(int moisture) {
  // Create JSON payload
  String jsonData = "{\"moisture\":" + String(moisture) + "}";
  
  // Start TCP connection to ThingsBoard
  String startConn = "AT+CIPSTART=\"TCP\",\"" + thingsboardHost + "\"," + thingsboardPort;
  sendCommand(startConn, 3000);
  
  // Prepare HTTP POST request
  String httpRequest = "POST /api/v1/" + accessToken + "/telemetry HTTP/1.1\r\n";
  httpRequest += "Host: " + thingsboardHost + "\r\n";
  httpRequest += "Content-Type: application/json\r\n";
  httpRequest += "Content-Length: " + String(jsonData.length()) + "\r\n";
  httpRequest += "\r\n";
  httpRequest += jsonData;
  
  // Send data length
  String sendCmd = "AT+CIPSEND=" + String(httpRequest.length());
  ESP_SERIAL.println(sendCmd);
  delay(500);
  
  // Send actual data
  ESP_SERIAL.print(httpRequest);
  delay(2000);
  
  // Close connection
  sendCommand("AT+CIPCLOSE", 1000);
  
  DEBUG_SERIAL.println("Data sent!");
}