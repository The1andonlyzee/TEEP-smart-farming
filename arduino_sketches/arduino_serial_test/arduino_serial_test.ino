/*
 * Arduino Mega - Simple Serial Test
 * Tests if ESP32 can receive data from Arduino
 */

void setup() {
  Serial.begin(115200);   // Debug
  Serial1.begin(115200);  // To ESP32
  
  Serial.println("Arduino Serial Test");
  Serial.println("Sending 'TEST' every second to Serial1");
}

void loop() {
  Serial.println("Sending: TEST");
  Serial1.println("TEST");
  delay(1000);
}
