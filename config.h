#pragma once

// CAN adapter: uncomment for MCP2515 SPI adapter, otherwise Serial CAN bus
// Install "mcp_can" library. More at https://github.com/coryjfowler/MCP_CAN_lib
//#define USE_MCP_CAN_SPI

#define MCP_CAN_SPI_CS_PIN 10

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
    #define SPEED_CALIBRATION 30
#endif

#ifndef MAX_RPM
    #define MAX_RPM 8000
#endif

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
