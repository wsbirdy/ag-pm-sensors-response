#include <Arduino.h>
#include "PMS_custom.h"

#define UART2_RX            0
#define UART2_TX            1

#define PMS_SERIAL_PORT Serial1
PMS pms(PMS_SERIAL_PORT);
PMS::DATA data;

void setup()
{
    Serial.begin(115200);         // Debug Serial Terminal
    // This is the most important part for the ESP32-C3!
    unsigned long startWait = millis();
    while (!Serial && (millis() - startWait < 5000)) {
        delay(100); 
    }
    // PMS_SERIAL_PORT.begin(9600);  // Plantower Serial Port
    PMS_SERIAL_PORT.begin(9600, SERIAL_8N1, UART2_RX, UART2_TX);
    pms.activeMode();             // Switch to active mode

    Serial.println("Waking up, wait 30 seconds for stable readings...");
    pms.wakeUp();
    delay(30000);
}

void loop()
{
  Serial.println("Wait max. 1 second for read...");
  if (pms.readUntil(data))
  {
    Serial.print("Standard Particles: ");
    Serial.println();
    Serial.print("PM 1.0(ug/m3): ");
    Serial.println(data.PM_SP_UG_1_0);
    Serial.print("PM 2.5(ug/m3): ");
    Serial.println(data.PM_SP_UG_2_5);
    Serial.print("PM 10.0(ug/m3): ");
    Serial.println(data.PM_SP_UG_10_0);
    Serial.println();

    Serial.print("Atmospheric Environment: ");
    Serial.println();
    Serial.print("PM 1.0(ug/m3): ");
    Serial.println(data.PM_AE_UG_1_0);
    Serial.print("PM 2.5(ug/m3): ");
    Serial.println(data.PM_AE_UG_2_5);
    Serial.print("PM 10.0(ug/m3): ");
    Serial.println(data.PM_AE_UG_10_0);
    Serial.println();

    Serial.print("Number of Particles: ");
    Serial.println();
    Serial.print("PM 0.3: ");
    Serial.println(data.PM_PC_0_3);
    Serial.print("PM 0.5: ");
    Serial.println(data.PM_PC_0_5);
    Serial.print("PM 1.0: ");
    Serial.println(data.PM_PC_1_0);
    Serial.print("PM 2.5: ");
    Serial.println(data.PM_PC_2_5);
    Serial.print("PM 5.0: ");
    Serial.println(data.PM_PC_5_0);
    Serial.print("PM 10.0: ");
    Serial.println(data.PM_PC_10_0);
    Serial.println("--------------------------------------");
  }
  else
  {
    Serial.println("No data.");
  }
  delay(1000);
}