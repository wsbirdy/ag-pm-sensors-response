#include "cubicPmUart.h"

#define cmd_readParticleMeasurement         0x0b
#define cmd_enableParticleMeasurement       0x0c
#define cmd_setupReadMeasurementTime        0x0d
#define cmd_setupReadTimingMode             0x05
#define cmd_setupReadWorkingMode            0x06
#define cmd_setupReadCalibratedCoefficient  0x07
#define cmd_enableLaserDiode                0x08
#define cmd_softwareVersion                 0x1e
#define cmd_serialNumber                    0x1f


PM2012::PM2012(Stream& serial) : _serial(serial) {}

bool PM2012::readMeasurement(PMData& data) {
    // Command to read concentration and particle number
    uint8_t cmd[] = {0x11, 0x02, cmd_readParticleMeasurement, 0x07, 0xDB};
    _serial.write(cmd, 5);

    uint8_t response[56];
    memset(response, 0, 56);

    // Wait for response (Start: 0x16, Length: 0x35 (53 bytes))
    if (_serial.readBytes(response, 56) != 56) return false;
    if (response[0] != 0x16 || response[1] != 0x35 || response[2] != 0x0B) return false;

    // Verify Checksum: sum of bytes 0 to 54 + byte 55 should = 256 (0x00 in 8-bit)
    uint8_t sum = 0;
    for (int i = 0; i < 55; i++) sum += response[i];
    if ((uint8_t)(256 - sum) != response[55]) return false;
    
    // Serial.printf("Measurement raw data: ");
    // for (int i = 0; i < sizeof(response); i++) {
    //     Serial.printf("%02x ", response[i]);
    // }

    // Data Mapping (4 bytes per value, Big Endian)
    data.pm1_0_grimm = parseUint32(&response[3]);
    data.pm2_5_grimm = parseUint32(&response[7]);
    data.pm10_grimm  = parseUint32(&response[11]);
    data.pm1_0_tsi   = parseUint32(&response[15]);
    data.pm2_5_tsi   = parseUint32(&response[19]);
    data.pm10_tsi    = parseUint32(&response[23]);
    data.count_0_3   = parseUint32(&response[27]);
    data.count_0_5   = parseUint32(&response[31]);
    data.count_1_0   = parseUint32(&response[35]);
    data.count_2_5   = parseUint32(&response[39]);
    data.count_5_0   = parseUint32(&response[43]);
    data.count_10    = parseUint32(&response[47]);

    return true;
}

bool PM2012::openParticleMeasurement(void){
    uint8_t cmd[] = {0x11, 0x03, cmd_enableParticleMeasurement, 0x02, 0x1E, 0xC0}; // Checksum pre-calculated
    return _sendCommand(cmd, 5);
}

bool PM2012::closeParticleMeasurement(void){
    uint8_t cmd[] = {0x11, 0x03, cmd_enableParticleMeasurement, 0x01, 0x1E, 0xC1}; // Checksum pre-calculated
    return _sendCommand(cmd, 5);
}

bool PM2012::openFanAndLaser() {
    uint8_t cmd[] = {0x11, 0x03, 0x03, 0x01, 0xE8}; // Checksum pre-calculated
    return _sendCommand(cmd, 5);
}

bool PM2012::closeFanAndLaser() {
    uint8_t cmd[] = {0x11, 0x03, 0x03, 0x00, 0xE9}; // Checksum pre-calculated
    return _sendCommand(cmd, 5);
}

bool PM2012::getSoftwareVersion(char* version) {
    uint8_t cmd[] = {0x11, 0x01, 0x1E, 0xD0};
    _serial.write(cmd, 4);
    uint8_t resp[20];
    if (_serial.readBytes(resp, 20) > 0 && resp[2] == 0x1E) {
        memcpy(version, &resp[3], resp[1] - 1);
        return true;
    }
    return false;
}

bool PM2012::getSerialNumber(char* version) {
    uint8_t cmd[] = {0x11, 0x01, 0x1F, 0xCF};
    _serial.write(cmd, 4);
    uint8_t resp[20];
    if (_serial.readBytes(resp, 20) > 0 && resp[2] == 0x1E) {
        memcpy(version, &resp[3], resp[1] - 1);
        return true;
    }
    return false;
}

uint32_t PM2012::parseUint32(uint8_t* buf) {
    return ((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16) | ((uint32_t)buf[2] << 8) | buf[3];
}

uint8_t PM2012::calculateChecksum(uint8_t* buf, uint8_t len) {
    uint8_t sum = 0;
    for (uint8_t i = 0; i < len; i++) {
        sum += buf[i];
    }
    return (uint8_t)(256 - sum);
}

bool PM2012::_sendCommand(uint8_t* cmd, uint8_t len) {
    // Frame: [HEAD][LEN][CMD][DATA][CHKSUM]

    // 1. Clear any old data in the serial buffer
    while(_serial.available()) _serial.read();

    // 2. Send the command bytes
    _serial.write(cmd, len);

    // 3. The sensor always responds with a 4-byte ACK for commands
    // Format: 0x16 (Header) + 0x02 (Length) + Command + CS (Checksum)
    uint8_t ack[4];
    if (_serial.readBytes(ack, 4) == 4) {
        if (ack[0] == 0x16 && ack[2] == cmd[2]) {
            // Verify Checksum: (Sum of first 3 bytes + CS) % 256 should be 0
            uint8_t sum = ack[0] + ack[1] + ack[2];
            if ((uint8_t)(256 - sum) == ack[3]) {
                return true; // Command successful!
            }
        }
    }
    return false; // Sensor failed to acknowledge
}