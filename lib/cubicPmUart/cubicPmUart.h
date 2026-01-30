#ifndef PM2012_H
#define PM2012_H

#include <Arduino.h>

struct PMData {
    uint32_t pm1_0_grimm;
    uint32_t pm2_5_grimm;
    uint32_t pm10_grimm;
    uint32_t pm1_0_tsi;
    uint32_t pm2_5_tsi;
    uint32_t pm10_tsi;
    uint32_t count_0_3;
    uint32_t count_0_5;
    uint32_t count_1_0;
    uint32_t count_2_5;
    uint32_t count_5_0;
    uint32_t count_10;
};

class PM2012 {
public:
    PM2012(Stream& serial);
    ~PM2012(){};
    void begin(HardwareSerial& serial) {_serial = serial;}
    bool readMeasurement(PMData& data);
    bool openParticleMeasurement(void);
    bool closeParticleMeasurement(void);
    bool getSoftwareVersion(char* version);
    bool getSerialNumber(char* version);
    bool openFanAndLaser();
    bool closeFanAndLaser();

private:
    Stream& _serial;
    uint8_t calculateChecksum(uint8_t* buf, uint8_t len);
    uint32_t parseUint32(uint8_t* buf);
    bool _sendCommand(uint8_t* cmd, uint8_t len);
};

#endif