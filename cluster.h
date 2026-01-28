#pragma once

#define NUMBER_OF_GEARS 7
#define MAX_SPEED_KMH_X10 2800u // e.g. 2600u for 260 km/h
#define SPEED_CALIBRATION 30 // This value is empirically set so the speed matches on this particular cluster
#define MAX_RPM 8000

// Enable if you get a cruise control warning
//#define CAN_CRUISE_ALT
