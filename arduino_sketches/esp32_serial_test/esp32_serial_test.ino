/*
 * ESP32 - Simple Serial Test
 * Tests if ESP32 can receive data from Arduino
 */

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=== ESP32 Serial Test ===");
  Serial.println("Waiting to receive 'TEST' from Arduino...");
}

void loop() {
  if (Serial.available()) {
    String received = Serial.readStringUntil('\n');
    Serial.print("RECEIVED: '");
    Serial.print(received);
    Serial.println("'");
    
    if (received == "TEST") {
      Serial.println("✓✓✓ COMMUNICATION WORKING! ✓✓✓");
    } else {
      Serial.println("⚠️  Data corrupted or incorrect");
      Serial.print("Expected: TEST, Got: ");
      Serial.println(received);
    }
  }
}
