#include <Arduino.h>
// #include "PMS.h"
#include "SensirionUartSps30.h"

#define DEBUG_OUT Serial
#define DEBUG_OUT_BAUD 115200

#define SENSOR_SERIAL_INTERFACE Serial0
#define PMSA003I_ADDRESS 0x12
#define LED_PIN 10  // Standard for AirGradient/C3-Mini
#define NO_ERROR 0
SensirionUartSps30 sps30_sensor;
// PMS pmsa(Serial0); //Serial 0 is used for communication with PMS sensor
// PMS::DATA data;

static char errorMessage[64];
static int16_t error;

void setup() {
  // 1. Initialize Serial at 115200 baud
  DEBUG_OUT.begin(DEBUG_OUT_BAUD);

  // 2. WAIT for the Serial Monitor to be opened (Max 5 seconds)
  // This is the most important part for the ESP32-C3!
  unsigned long startWait = millis();
  while (!DEBUG_OUT && (millis() - startWait < 5000)) {
    delay(100); 
  }

  SENSOR_SERIAL_INTERFACE.begin(DEBUG_OUT_BAUD);
  if(SENSOR_SERIAL_INTERFACE){
    DEBUG_OUT.println("Serial0 initialized successfully.");
  } else {
    DEBUG_OUT.println("Serial0 initialization failed!");
  }

  // 3. Initialize the LED
  pinMode(LED_PIN, OUTPUT);

  DEBUG_OUT.println("--- System Starting ---");
  DEBUG_OUT.println("ESP32-C3 Serial initialized successfully.");

  sps30_sensor.begin(SENSOR_SERIAL_INTERFACE);

  sps30_sensor.stopMeasurement();
  int8_t serialNumber[32] = {0};
  int8_t productType[9] = {0};

  error = sps30_sensor.readSerialNumber(serialNumber, 32);
  if (error != NO_ERROR) {
      DEBUG_OUT.print("Error trying to execute readSerialNumber(): ");
      errorToString(error, errorMessage, sizeof errorMessage);
      DEBUG_OUT.println(errorMessage);
  }
  DEBUG_OUT.print("serialNumber: ");
  DEBUG_OUT.print((const char*)serialNumber);
  DEBUG_OUT.println();
  delay(100);

  while(SENSOR_SERIAL_INTERFACE.available()){}
  error = sps30_sensor.readProductType(productType, 9);
  if (error != NO_ERROR) {
      DEBUG_OUT.printf("Error trying to execute readProductType(): %x", error);
      errorToString(error, errorMessage, sizeof errorMessage);
      DEBUG_OUT.println(errorMessage);
  }
  DEBUG_OUT.print("productType: ");
  DEBUG_OUT.print((const char*)productType);
  DEBUG_OUT.println();
  delay(100);

  error = sps30_sensor.startMeasurement((SPS30OutputFormat)(261));
  if (error != NO_ERROR) {
      DEBUG_OUT.print("Error trying to execute startMeasurement(): ");
      errorToString(error, errorMessage, sizeof errorMessage);
      DEBUG_OUT.println(errorMessage);
  }
  SENSOR_SERIAL_INTERFACE.flush();
  delay(3000);
}

int counter = 0;

void loop() {

  uint16_t mc1p0 = 0;
  uint16_t mc2p5 = 0;
  uint16_t mc4p0 = 0;
  uint16_t mc10p0 = 0;
  uint16_t nc0p5 = 0;
  uint16_t nc1p0 = 0;
  uint16_t nc2p5 = 0;
  uint16_t nc4p0 = 0;
  uint16_t nc10p0 = 0;
  uint16_t typicalParticleSize = 0;

    // Print a message every second
  DEBUG_OUT.print("Sensor Heartbeat #");
  DEBUG_OUT.println(counter);

  // Blink the LED so we know the code is running
  digitalWrite(LED_PIN, HIGH);
  delay(100);
  digitalWrite(LED_PIN, LOW);

  counter++;
  delay(1400); // Total 1.5 second delay

  while(DEBUG_OUT.available()) DEBUG_OUT.read();
  while(SENSOR_SERIAL_INTERFACE.available()) SENSOR_SERIAL_INTERFACE.read();

  error = sps30_sensor.readMeasurementValuesUint16(mc1p0, mc2p5, mc4p0, mc10p0,
                                        nc0p5, nc1p0, nc2p5, nc4p0,
                                        nc10p0, typicalParticleSize);

  if (error != NO_ERROR) {
      DEBUG_OUT.print("Error trying to execute readMeasurementValuesUint16(): ");
      errorToString(error, errorMessage, sizeof errorMessage);
      DEBUG_OUT.println(errorMessage);
      return;
  }
  DEBUG_OUT.print("mc1p0: ");
  DEBUG_OUT.print(mc1p0);
  DEBUG_OUT.print("\t");
  DEBUG_OUT.print("mc2p5: ");
  DEBUG_OUT.print(mc2p5);
  DEBUG_OUT.print("\t");
  DEBUG_OUT.print("mc4p0: ");
  DEBUG_OUT.print(mc4p0);
  DEBUG_OUT.print("\t");
  DEBUG_OUT.print("mc10p0: ");
  DEBUG_OUT.print(mc10p0);
  DEBUG_OUT.print("\t");
  DEBUG_OUT.print("nc0p5: ");
  DEBUG_OUT.print(nc0p5);
  DEBUG_OUT.print("\t");
  DEBUG_OUT.print("nc1p0: ");
  DEBUG_OUT.print(nc1p0);
  DEBUG_OUT.print("\t");
  DEBUG_OUT.print("nc2p5: ");
  DEBUG_OUT.print(nc2p5);
  DEBUG_OUT.print("\t");
  DEBUG_OUT.print("nc4p0: ");
  DEBUG_OUT.print(nc4p0);
  DEBUG_OUT.print("\t");
  DEBUG_OUT.print("nc10p0: ");
  DEBUG_OUT.print(nc10p0);
  DEBUG_OUT.print("\t");
  DEBUG_OUT.print("typicalParticleSize: ");
  DEBUG_OUT.print(typicalParticleSize);
  DEBUG_OUT.println();

}

