#include "config.h"

#if defined(USE_MCP_CAN_SPI)

#include <Arduino.h>
#include <SPI.h>
#include <mcp_can.h>
#include "can_adapter.h"
#include "serial.h"

static MCP_CAN CAN(MCP_CAN_SPI_CS_PIN);

void canBegin() {
    randomSeed(analogRead(A0));
    while (CAN_OK != CAN.begin(MCP_STDEXT, CAN_100KBPS, MCP_CAN_SPI_SPEED)) {
        pc.println("CAN BUS init fail, retrying...");
        delay(100);
    }
    CAN.setMode(MCP_NORMAL);
}

void canSend(uint32_t id, const uint8_t* data) {
    CAN.sendMsgBuf(id, 0, 8, (byte*)data);
}

void canPoll(const CanHandlerEntry* handlers, size_t count) {
    unsigned long id;
    uint8_t len;
    uint8_t buf[8];
    while (CAN_MSGAVAIL == CAN.checkReceive()) {
        CAN.readMsgBuf(&id, &len, buf);
        for (size_t i = 0; i < count; ++i) {
            if (id == handlers[i].id) {
                handlers[i].handler(buf);
                break;
            }
        }
    }
}

#endif
