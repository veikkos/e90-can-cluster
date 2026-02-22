#include <Arduino.h>

#define VERSION 'j'
#define DEVICE_NAME "E90 Cluster (veikkos)"
#define DEVICE_UNIQUE_ID "55324453-cb75-50da-e080-78e37b21c548"

#ifndef SIGNATURE_0
	#define SIGNATURE_0 'N'
#endif
#ifndef SIGNATURE_1
	#define SIGNATURE_1 '/'
#endif
#ifndef SIGNATURE_2
	#define SIGNATURE_2 'A'
#endif

#include "FlowSerialRead.h"
#include "SHCustomProtocol.h"
SHCustomProtocol shCustomProtocol;
#include "SHCommands.h"

char loop_opt;
unsigned long lastSerialActivity = 0;

void simHubIdle(bool critical) {
    shCustomProtocol.idle();
}

void simHubSetup()
{
	FlowSerialBegin(19200);

	shCustomProtocol.setup();
	arqserial.setIdleFunction(simHubIdle);
}

void simHubSerialRead() {
	shCustomProtocol.loop();

	// Wait for data
	if (FlowSerialAvailable() > 0) {
		if (FlowSerialTimedRead() == MESSAGE_HEADER)
		{
			lastSerialActivity = millis();
			// Read command
			loop_opt = FlowSerialTimedRead();

			if (loop_opt == '1') Command_Hello();
			else if (loop_opt == '8') Command_SetBaudrate();
			else if (loop_opt == 'J') Command_ButtonsCount();
			else if (loop_opt == '2') Command_TM1638Count();
			else if (loop_opt == 'B') Command_SimpleModulesCount();
			else if (loop_opt == 'A') Command_Acq();
			else if (loop_opt == 'N') Command_DeviceName();
			else if (loop_opt == 'I') Command_UniqueId();
			else if (loop_opt == '0') Command_Features();
			else if (loop_opt == '4') Command_RGBLEDSCount();
			else if (loop_opt == '6') FlowSerialWrite(0x15); // RGBLEDSData ack
			else if (loop_opt == 'R') FlowSerialWrite(0x15); // RGBMatrixData ack
			else if (loop_opt == 'P') Command_CustomProtocolData();
			else if (loop_opt == 'X')
			{
				String xaction = FlowSerialReadStringUntil(' ', '\n');
				if (xaction == F("list")) Command_ExpandedCommandsList();
				else if (xaction == F("mcutype")) Command_MCUType();
			}
		}
	}
}
