#include <Arduino.h>
#include <Wire.h>

// On the ESP32-C3, the default I2C pins are usually:
// SDA: GPIO 8
// SCL: GPIO 9
// You can change these in Wire.begin(SDA, SCL) if needed.
#define I2C_SDA_PIN   7
#define I2C_SCL_PIN   6
#define RESET_PIN     4
#define SET_PIN       5
#define COMM_SEL_PIN  3

void setup() {
    Serial.begin(115200);
    while (!Serial); // Wait for Serial Monitor to open

    pinMode(RESET_PIN,OUTPUT);
    pinMode(SET_PIN,OUTPUT);
    pinMode(COMM_SEL_PIN,OUTPUT);

    digitalWrite(RESET_PIN,HIGH);
    digitalWrite(SET_PIN,HIGH);
    digitalWrite(COMM_SEL_PIN,LOW);

    Serial.println("\nI2C Scanner Initializing...");

    // Initialize I2C with default pins
    if (!Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN)) {
        Serial.println("Failed to initialize I2C bus!");
        while (1);
    }

    Serial.println("Scanning...");
}

void loop() {
  byte error, address;
  int nDevices = 0;

  for (address = 1; address < 127; address++) {
    // The i2c_scanner uses the return value of
    // the Write.endTransmission to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    // Serial.printf("I2C error: %d\n", error);
    
    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address < 16) Serial.print("0");
      Serial.print(address, HEX);
      Serial.println("  !");

      nDevices++;
    } else if (error == 4) {
      Serial.print("Unknown error at address 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
    }
  }

  if (nDevices == 0) {
    Serial.println("No I2C devices found\n");
  } else {
    Serial.println("Scan complete.\n");
  }

  delay(5000); // Wait 5 seconds for next scan
}