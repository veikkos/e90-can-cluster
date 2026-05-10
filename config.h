#pragma once

// CAN adapter: pick one option below, or none for the Serial CAN bus default.
// USE_MCP_CAN_SPI and USE_ESP32_TWAI are mutually exclusive.

// MCP2515 SPI adapter. Install "mcp_can" library.
// More at https://github.com/coryjfowler/MCP_CAN_lib
//#define USE_MCP_CAN_SPI

#ifndef MCP_CAN_SPI_SPEED
    // Set to MCP_16MHZ if you have an adapter with 16MHz crystal
    #define MCP_CAN_SPI_SPEED MCP_8MHZ
#endif

#ifndef MCP_CAN_SPI_CS_PIN
    #define MCP_CAN_SPI_CS_PIN 10
#endif

// ESP32 built-in TWAI controller with an SN65HVD230 (or compatible) transceiver.
// No external library required; uses the TWAI driver from the ESP32 Arduino core.
//#define USE_ESP32_TWAI

#ifndef TWAI_TX_PIN
    #define TWAI_TX_PIN 23
#endif

#ifndef TWAI_RX_PIN
    #define TWAI_RX_PIN 22
#endif

// Serial protocol: uncomment for SimHub, otherwise custom binary
//#define USE_SIMHUB

// Serial baud rates
#ifndef PC_SERIAL_BAUD
    #define PC_SERIAL_BAUD 921600
#endif

#ifndef CAN_SERIAL_BAUD
    #define CAN_SERIAL_BAUD 115200
#endif

// Cluster parameters
#ifndef NUMBER_OF_GEARS
    #define NUMBER_OF_GEARS 7
#endif

#ifndef MAX_SPEED_KMH_X10
    #define MAX_SPEED_KMH_X10 2800u  // 2800 = 280 km/h
#endif

#ifndef SPEED_CALIBRATION
    // 60 = reduces indicated speed by ~6% to compensate for the built-in speedometer
    // padding (law requires speedometers to never read below actual speed).
    // With 0: odometer is accurate, speedometer reads a bit high (factory behavior).
    // With 60: speedometer reads close to true speed, but odometer counts a bit slow.
    // Error can be fully removed by removing the built-in error by "coding".
    #define SPEED_CALIBRATION 60
#endif

#ifndef MAX_RPM
    #define MAX_RPM 8000
#endif

// Comment away to hide "SPORT" from the sport gear mode
#define GEAR_SPORT_TEXT

// Enable to show "S1", "S2" instead of "D1", "D2" in sport gear mode
//#define GEAR_SPORT_S

// Comment away to show "DS" in sport mode, use with GEAR_SPORT_S
#define GEAR_EXPLICIT_NUMBER_IN_SPORT

// Uncomment if you get cruise control warnings
//#define CAN_CRUISE_ALT

// Hardware
#ifndef REFUELING_LED_PIN
    #define REFUELING_LED_PIN 33
#endif

// AD5272 digital potentiometer for ambient temperature
//#define USE_AD5272_AMBIENT

#ifndef AD5272_I2C_ADDRESS
    #define AD5272_I2C_ADDRESS 0x2E
#endif

#ifndef AD5272_I2C_SDA_PIN
    #define AD5272_I2C_SDA_PIN 18
#endif

#ifndef AD5272_I2C_SCL_PIN
    #define AD5272_I2C_SCL_PIN 19
#endif

// Debug: uncomment to log CAN frames from cluster
//#define READ_FRAMES_FROM_CLUSTER_1B4  // Speed & handbrake
//#define READ_FRAMES_FROM_CLUSTER_2C0  // Brightness/light sensor
//#define READ_FRAMES_FROM_CLUSTER_2CA  // Outside temperature
//#define READ_FRAMES_FROM_CLUSTER_2F8  // Time and date
