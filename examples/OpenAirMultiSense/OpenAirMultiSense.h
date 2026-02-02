#ifndef OpenAirMultiSense_h
#define OpenAirMultiSense_h

#include <Wire.h>
#include "Adafruit_PM25AQI.h"
#include "SensirionUartSps30.h"
#include "pm2008_i2c.h"
#include "cubicPmUart.h"
#include "FS.h"
#include "LittleFS.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

// AirGradient Open Air ESP32C3 - Pin Map
#define UART2_RX            0
#define UART2_TX            1
#define WATCHDOG_DONE_PIN   2
#define PMSA003I_SET_PIN    3
#define PMSA003I_RESET_PIN  4
#define I2C_SDA_PIN         7
#define I2C_SCL_PIN         6
#define BUTTON_PIN          9
#define LED_PIN             10  // Standard for AirGradient/C3-Mini

// I2C Address is usually 0x3C or 0x3D
#define i2c_Address 0x3C 
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1   // Set to -1 if your display doesn't have a reset pin

struct SensirionMeasurement {
    uint16_t mc1p0;
    uint16_t mc2p5;
    uint16_t mc4p0;
    uint16_t mc10p0;
    uint16_t nc0p5;
    uint16_t nc1p0;
    uint16_t nc2p5;
    uint16_t nc4p0;
    uint16_t nc10p0;
    uint16_t typicalParticleSize;
};

// Log only PM2.5 or above particles size
struct __attribute__((packed)) SensorData {
    uint16_t particles;         // 2 bytes for number of particles
    uint16_t concentration;     // 2 bytes for concentration in µg/m3
};

// Total size = 30 bytes
struct __attribute__((packed)) SensorPayload {
    char header[2] = {0x4f, 0x41};  // 2 bytes
    uint32_t counter;               // 4 bytes
    uint32_t timestamp;             // 4 bytes  [ms]
    struct SensorData sps30Data;    // 4 bytes from SPS30
    struct SensorData pmsa003iData; // 4 bytes from PMSA003I
    struct SensorData cubicPm2012;  // 4 bytes from Cubic PM2012 [GRIMM]
    uint16_t cubicPm2012Tsi;        // 2 bytes from Cubic PM2012 [TSI concentration]
    struct SensorData cubicPm2016;  // 4 bytes from Cubic PM2016
    char terminater[2] = {0xaa, 0xbb};    // 2 bytes
};

#endif  // OpenAirMultiSense.h