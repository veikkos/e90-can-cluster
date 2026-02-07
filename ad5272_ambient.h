#pragma once

#include "config.h"
#include <Arduino.h>

#if defined(USE_AD5272_AMBIENT)

#include <Wire.h>

#define AD5272_CMD_WRITE_RDAC       (1 << 2)
#define AD5272_CMD_READ_RDAC        (2 << 2)
#define AD5272_CMD_STORE_WIPER      (3 << 2)
#define AD5272_CMD_SW_RESET         (4 << 2)
#define AD5272_CMD_READ_MEM         (5 << 2)
#define AD5272_CMD_READ_50TP_LAST   (6 << 2)
#define AD5272_CMD_WRITE_CONTROL    (7 << 2)
#define AD5272_CMD_READ_CONTROL     (8 << 2)

// Control register bits
#define AD5272_CONTROL_50TP_ENABLE          0x01  // Enable 50-TP EEPROM writes
#define AD5272_CONTROL_RDAC_WRITE_ENABLE    0x02  // Enable RDAC wiper writes
#define AD5272_CONTROL_RDAC_CALIB_DISABLE   0x04  // Disable calibration (normally enabled)

#define AD5272_MAX_POSITION 1023

class AD5272Ambient {
public:
    AD5272Ambient();

    bool begin(uint8_t i2c_address = AD5272_I2C_ADDRESS);
    bool setTemperature(float celsius);
    bool storeStartupTemperature(float celsius);
    uint16_t getWiperPosition();
    uint16_t readEEPROM();
    uint16_t readControlRegister();
    bool setWiperPosition(uint16_t position);

private:
    uint8_t _address;
    bool _initialized;

    float celsiusToResistance(float celsius);
    uint16_t resistanceToWiperPosition(float resistance_kohm);
    bool writeCommand(uint8_t command, uint16_t data);
    uint16_t readRegister(uint8_t command);
};

#endif
