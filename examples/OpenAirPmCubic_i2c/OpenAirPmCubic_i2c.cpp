/*
*   The datasheet of PM2016 from The Cubic Sensor and Instrument Co., Ltd 
*   is not compatible with the firmware inside of the aqiured sensors. The
*   firmware of pm2008 module is properly function with the PM2016 sensor.
*
*/

#include <Arduino.h>
#include <pm2008_i2c.h>
#include <cubicPmUart.h>
#include <Wire.h>

// #define PM2016_UART

#ifdef PM2016_UART
#define UART_MODE
#else
#define I2C_MODE
#endif

#define I2C_SDA_PIN 7
#define I2C_SCL_PIN 6
#define RESET_PIN     4
#define SET_PIN       5
#define COMM_SEL_PIN  3
#define UART2_RX    0
#define UART2_TX    1
#define PM2012_SERIAL_PORT  Serial1

PM2008_I2C pm2008_i2c;
PM2012 pm_uart(PM2012_SERIAL_PORT);

void setup() {

    pinMode(RESET_PIN,OUTPUT);
    pinMode(SET_PIN,OUTPUT);
    pinMode(COMM_SEL_PIN,OUTPUT);

    digitalWrite(RESET_PIN,HIGH);
    digitalWrite(SET_PIN,HIGH);
    digitalWrite(COMM_SEL_PIN,LOW);

//   pm2008_i2c.begin();
    Serial.begin(115200);
    unsigned long startWait = millis();
    while (!Serial && (millis() - startWait < 5000)) {
        delay(100); 
    }
    Serial.println("Serial initialized successfully.");

#ifdef I2C_MODE
    if (!Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN)) {
        Serial.println("Failed to initialize I2C bus!");
        while (1);
    }else{
        Serial.println("Succesfully initialize I2C!!");
    }
    pm2008_i2c.command();
#else
    PM2012_SERIAL_PORT.begin(9600, SERIAL_8N1, UART2_RX, UART2_TX);
    startWait = millis();
    while (!PM2012_SERIAL_PORT && (millis() - startWait < 5000)) {
        delay(100); 
    }
    Serial.println("Cubic PM UART sensor initialize.");

    char serialNumber[13] = {0};
    char fwVersion[10] = {0};
    pm_uart.getSerialNumber(serialNumber);
    Serial.print("serialNumber: ");
    Serial.print((const char*)serialNumber);
    Serial.println();
    delay(100);

    pm_uart.getSoftwareVersion(fwVersion);
    Serial.print("softwareVersion: ");
    Serial.print((const char*)serialNumber);
    Serial.println();

#endif

    delay(1000);
}

void loop() {


    #ifdef I2C_MODE
    uint8_t ret = pm2008_i2c.read();
    if (ret == 0) {
        Serial.print("\nSuccesfully Reading....\n");
        Serial.printf("PM 1.0 (GRIMM) : %d\t, PM 2.5 (GRIMM) : %d\t, PM 10 (GRIMM) : %d \n", 
                        pm2008_i2c.pm1p0_grimm, pm2008_i2c.pm2p5_grimm, pm2008_i2c.pm1p0_grimm);
        Serial.printf("PM 1.0 (TSI) : %d\t, PM 2.5 (TSI) : %d\t, PM 10 (TSI) : %d \n", 
                        pm2008_i2c.pm1p0_tsi, pm2008_i2c.pm2p5_tsi, pm2008_i2c.pm10_tsi);
        Serial.printf("Particles of 0.3 um:  %d\t, 0.5 um:  %d\t, 1.0 um: %d, 2.5 um: %d\t, 5.0 um: %d, 10.0 um:%d \n", 
                        pm2008_i2c.number_of_0p3_um, pm2008_i2c.number_of_0p5_um, pm2008_i2c.number_of_1_um, 
                        pm2008_i2c.number_of_2p5_um, pm2008_i2c.number_of_5_um, pm2008_i2c.number_of_10_um);
    }else{
        Serial.println("\nRead Failed....\n");
    }

    #else
    PMData data;
    bool ret = pm_uart.readMeasurement(data);
    if (ret == true) {
        Serial.print("\nSuccesfully Reading....\n");
        Serial.printf("PM 1.0 (GRIMM) : %d\t, PM 2.5 (GRIMM) : %d\t, PM 10 (GRIMM) : %d \n", 
                        data.pm1_0_grimm, data.pm2_5_grimm, data.pm10_grimm);
        Serial.printf("PM 1.0 (TSI) : %d\t, PM 2.5 (TSI) : %d\t, PM 10 (TSI) : %d \n", 
                        data.pm1_0_tsi, data.pm2_5_tsi, data.pm10_tsi);
        Serial.printf("Particles of 0.3 um:  %d\t, 0.5 um:  %d\t, 1.0 um: %d, 2.5 um: %d\t, 5.0 um: %d, 10.0 um:%d \n", 
                        data.count_0_3, data.count_0_5, data.count_1_0, 
                        data.count_2_5, data.count_5_0, data.count_10);
    }else{
        Serial.println("\nRead Failed....\n");
    }
    #endif
    
    delay(1000);

}
