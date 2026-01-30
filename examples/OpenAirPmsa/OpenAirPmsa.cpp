#include <Arduino.h>
#include <Wire.h>
#include "Adafruit_PM25AQI.h"

#define DEBUG_OUT Serial
#define DEBUG_OUT_BAUD 115200

#define I2C_SDA_PIN 7
#define I2C_SCL_PIN 6

#define LED_PIN 10  // Standard for AirGradient/C3-Mini

Adafruit_PM25AQI aqi = Adafruit_PM25AQI();

void setup() {
    // Wait for serial monitor to open
    DEBUG_OUT.begin(DEBUG_OUT_BAUD);

    // This is the most important part for the ESP32-C3!
    unsigned long startWait = millis();
    while (!DEBUG_OUT && (millis() - startWait < 5000)) {
    delay(100); 
    }

    DEBUG_OUT.println("Adafruit PMSA003I Air Quality Sensor");
    // There are 3 options for connectivity!
    if (!Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN)) {
    Serial.println("Failed to initialize I2C bus!");
    while (1);
    }
    if (! aqi.begin_I2C()) {      // connect to the sensor over I2C
    DEBUG_OUT.println("Could not find PM 2.5 sensor!");
    while (1) delay(10);
    }
    DEBUG_OUT.println("PM25 found!");

    // 3. Initialize the LED
    pinMode(LED_PIN, OUTPUT);
}

int counter = 0;

void loop() {
    
    // Blink the LED so we know the code is running
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);

    counter++;
    delay(400); // Total 0.5 second delay


    PM25_AQI_Data data;

    if (! aqi.read(&data)) {
    Serial.println("Could not read from AQI");
    delay(500);  // try again in a bit!
    return;
    }
    DEBUG_OUT.print("AQI reading success - ");
    DEBUG_OUT.printf("Sensor Heartbeat #%d \n", counter);

    DEBUG_OUT.println();
    DEBUG_OUT.println(F("---------------------------------------"));
    DEBUG_OUT.println(F("Concentration Units (standard)"));
    DEBUG_OUT.println(F("---------------------------------------"));
    DEBUG_OUT.print(F("PM 1.0: ")); DEBUG_OUT.print(data.pm10_standard);
    DEBUG_OUT.print(F("\t\tPM 2.5: ")); DEBUG_OUT.print(data.pm25_standard);
    DEBUG_OUT.print(F("\t\tPM 10: ")); DEBUG_OUT.println(data.pm100_standard);
    DEBUG_OUT.println(F("Concentration Units (environmental)"));
    DEBUG_OUT.println(F("---------------------------------------"));
    DEBUG_OUT.print(F("PM 1.0: ")); DEBUG_OUT.print(data.pm10_env);
    DEBUG_OUT.print(F("\t\tPM 2.5: ")); DEBUG_OUT.print(data.pm25_env);
    DEBUG_OUT.print(F("\t\tPM 10: ")); DEBUG_OUT.println(data.pm100_env);
    DEBUG_OUT.println(F("---------------------------------------"));
    DEBUG_OUT.print(F("Particles > 0.3um / 0.1L air:")); DEBUG_OUT.println(data.particles_03um);
    DEBUG_OUT.print(F("Particles > 0.5um / 0.1L air:")); DEBUG_OUT.println(data.particles_05um);
    DEBUG_OUT.print(F("Particles > 1.0um / 0.1L air:")); DEBUG_OUT.println(data.particles_10um);
    DEBUG_OUT.print(F("Particles > 2.5um / 0.1L air:")); DEBUG_OUT.println(data.particles_25um);
    DEBUG_OUT.print(F("Particles > 5.0um / 0.1L air:")); DEBUG_OUT.println(data.particles_50um);
    DEBUG_OUT.print(F("Particles > 50 um / 0.1L air:")); DEBUG_OUT.println(data.particles_100um);
    DEBUG_OUT.println(F("---------------------------------------"));

    delay(500);
}


