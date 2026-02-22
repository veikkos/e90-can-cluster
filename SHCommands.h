#define MESSAGE_HEADER 0x03

void Command_Hello() {
	FlowSerialTimedRead();
	delay(10);
	FlowSerialPrint(VERSION);
	FlowSerialFlush();
}

void Command_SetBaudrate() {
	SetBaudrate();
}

void Command_ButtonsCount() {
	FlowSerialWrite((byte)0);
	FlowSerialFlush();
}

void Command_TM1638Count() {
	FlowSerialWrite((byte)0);
	FlowSerialFlush();
}

void Command_SimpleModulesCount() {
	FlowSerialWrite((byte)0);
	FlowSerialFlush();
}

void Command_Acq() {
	FlowSerialWrite(0x03);
	FlowSerialFlush();
}

void Command_DeviceName() {
	FlowSerialPrint(DEVICE_NAME);
	FlowSerialPrint("\n");
	FlowSerialFlush();
}

void Command_UniqueId() {
	FlowSerialPrint(DEVICE_UNIQUE_ID);
	FlowSerialPrint("\n");
	FlowSerialFlush();
}

void Command_MCUType() {
	FlowSerialPrint(SIGNATURE_0);
	FlowSerialPrint(SIGNATURE_1);
	FlowSerialPrint(SIGNATURE_2);
	FlowSerialFlush();
}

void Command_ExpandedCommandsList() {
	FlowSerialPrintLn("mcutype");
	FlowSerialPrintLn("keepalive");
	FlowSerialPrintLn();
	FlowSerialFlush();
}

void Command_Features() {
	delay(10);

	// Name
	FlowSerialPrint("N");

	// UniqueID
	FlowSerialPrint("I");

	// Custom Protocol Support
	FlowSerialPrint("P");

	FlowSerialPrint("\n");
	FlowSerialFlush();
}

void Command_RGBLEDSCount() {
	FlowSerialWrite((byte)0);
	FlowSerialFlush();
}

void Command_CustomProtocolData() {
	shCustomProtocol.read();
	FlowSerialWrite(0x15);
}
