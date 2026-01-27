#ifndef __SHCUSTOMPROTOCOL_H__
#define __SHCUSTOMPROTOCOL_H__

#include <Arduino.h>
#include "types.h"
#include "cluster.h"

extern SInput s_input;

class SHCustomProtocol {
private:

public:
	void setup() {
	}

	void read() {
		s_input.speed = FlowSerialReadStringUntil(';').toInt() * 10;
		s_input.rpm = FlowSerialReadStringUntil(';').toInt();
		s_input.oil_temp = FlowSerialReadStringUntil(';').toInt();
		s_input.fuel = FlowSerialReadStringUntil(';').toInt();
		
		String gearStr = FlowSerialReadStringUntil(';');
		if (gearStr == "N") {
			s_input.manualGear = NONE;
			s_input.currentGear = NEUTRAL;
		} else if (gearStr == "R") {
			s_input.manualGear = NONE;
			s_input.currentGear = REVERSE;
		} else {
			int gear = gearStr.toInt();
			s_input.manualGear = (GEAR_MANUAL)(min(gear, NUMBER_OF_GEARS));
			s_input.currentGear = DRIVE;
		}
		s_input.mode = NORMAL;

		s_input.water_temp = FlowSerialReadStringUntil(';').toInt();
		s_input.ignition = (IGNITION_STATE)FlowSerialReadStringUntil(';').toInt();
		s_input.light_lowbeam = s_input.ignition != IG_OFF;
		s_input.engine_running = FlowSerialReadStringUntil(';').toInt() != 0;
		
		// Indicators: 0=off, 1=left, 2=right, 3=hazard
		uint8_t indicators = FlowSerialReadStringUntil(';').toInt();
		s_input.indicator_state = (INDICATOR)indicators;
		
		s_input.handbrake = FlowSerialReadStringUntil(';').toInt() != 0;
		s_input.abs_warn = FlowSerialReadStringUntil('\n').toInt() != 0;
	}

	void loop() {
	}

	void idle() {
	}
};

#endif
