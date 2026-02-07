#include "ad5272_ambient.h"

#if defined(USE_AD5272_AMBIENT)

// NTC thermistor constants (BMW E90)
#define NTC_R0    5000.0f   // Ohms at 25C
#define NTC_T0    298.15f   // 25C in Kelvin
#define NTC_BETA  3950.0f

AD5272Ambient::AD5272Ambient() : _address(AD5272_I2C_ADDRESS), _initialized(false) {
}

bool AD5272Ambient::begin(uint8_t i2c_address) {
    _address = i2c_address;

    pinMode(AD5272_I2C_SDA_PIN, INPUT_PULLUP);
    pinMode(AD5272_I2C_SCL_PIN, INPUT_PULLUP);

    Wire.begin();

    uint16_t control = readRegister(AD5272_CMD_READ_CONTROL);
    if (control == 0xFFFF) {
        _initialized = false;
        return false;
    }

    // Enable RDAC wiper writing (required before any writes)
    if (!writeCommand(AD5272_CMD_WRITE_CONTROL, 0x02)) {
        _initialized = false;
        return false;
    }

    _initialized = true;
    return true;
}

bool AD5272Ambient::setTemperature(float celsius) {
    if (!_initialized) {
        return false;
    }

    celsius = constrain(celsius, -40.0, 80.0);
    float resistance = celsiusToResistance(celsius);
    uint16_t position = resistanceToWiperPosition(resistance);

    return setWiperPosition(position);
}

bool AD5272Ambient::storeStartupTemperature(float celsius) {
    if (!_initialized) {
        return false;
    }

    // Enable RDAC writing first (bit 1)
    if (!writeCommand(AD5272_CMD_WRITE_CONTROL, AD5272_CONTROL_RDAC_WRITE_ENABLE)) {
        return false;
    }

    delay(5);

    // Now set the wiper position
    if (!setTemperature(celsius)) {
        return false;
    }

    delay(10);  // Ensure RDAC write completes

    // Enable 50-TP (EEPROM) writing while keeping RDAC enabled
    uint8_t control_value = AD5272_CONTROL_50TP_ENABLE | AD5272_CONTROL_RDAC_WRITE_ENABLE;
    if (!writeCommand(AD5272_CMD_WRITE_CONTROL, control_value)) {
        return false;
    }

    delay(10);

    // Store current wiper position to EEPROM (50 write limit!)
    if (!writeCommand(AD5272_CMD_STORE_WIPER, 0x00)) {
        return false;
    }

    // Allow time for EEPROM write to complete
    delay(350);

    // Re-lock 50-TP (clear bit 0, keep bit 1)
    writeCommand(AD5272_CMD_WRITE_CONTROL, AD5272_CONTROL_RDAC_WRITE_ENABLE);

    return true;
}

float AD5272Ambient::celsiusToResistance(float celsius) {
    float temp_k = celsius + 273.15f;
    float resistance_ohms = NTC_R0 * expf(
        NTC_BETA * ((1.0f / temp_k) - (1.0f / NTC_T0))
    );
    return resistance_ohms / 1000.0f;
}

uint16_t AD5272Ambient::resistanceToWiperPosition(float resistance_kohm) {
    float position_normalized = resistance_kohm / 100.0;
    position_normalized = constrain(position_normalized, 0.0, 1.0);

    uint16_t position = (uint16_t)(position_normalized * AD5272_MAX_POSITION);

    return position;
}

bool AD5272Ambient::setWiperPosition(uint16_t position) {
    if (!_initialized) {
        return false;
    }

    position = min(position, AD5272_MAX_POSITION);

    return writeCommand(AD5272_CMD_WRITE_RDAC, position);
}

uint16_t AD5272Ambient::getWiperPosition() {
    if (!_initialized) {
        return 0;
    }

    uint16_t raw = readRegister(AD5272_CMD_READ_RDAC);
    return raw & 0x03FF;
}

uint16_t AD5272Ambient::readEEPROM() {
    if (!_initialized) {
        return 0;
    }

    uint16_t raw = readRegister(AD5272_CMD_READ_50TP_LAST);
    return raw & 0x03FF;
}

uint16_t AD5272Ambient::readControlRegister() {
    if (!_initialized) {
        return 0;
    }

    uint16_t raw = readRegister(AD5272_CMD_READ_CONTROL);
    return raw & 0x0F;
}

bool AD5272Ambient::writeCommand(uint8_t command, uint16_t data) {
    data = data & 0x03FF;
    uint8_t byte1 = command | ((data >> 8) & 0x03);
    uint8_t byte2 = data & 0xFF;

    Wire.beginTransmission(_address);
    Wire.write(byte1);
    Wire.write(byte2);
    uint8_t error = Wire.endTransmission();

    return (error == 0);
}

uint16_t AD5272Ambient::readRegister(uint8_t command) {
    Wire.beginTransmission(_address);
    Wire.write(command);
    Wire.write(0x00);

    if (Wire.endTransmission() != 0) {
        return 0xFFFF;
    }

    Wire.requestFrom(_address, (uint8_t)2);

    if (Wire.available() >= 2) {
        uint8_t msb = Wire.read();
        uint8_t lsb = Wire.read();
        return (((uint16_t)msb << 8) | lsb) & 0x03FF;
    }

    return 0xFFFF;
}

#endif // USE_AD5272_AMBIENT
