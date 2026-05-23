#include "config.h"

#if defined(USE_FLEXCAN_T4)

#include <Arduino.h>
#include <string.h>
#include <FlexCAN_T4.h>
#include "can_adapter.h"
#include "serial.h"

static FlexCAN_T4<FLEXCAN_T4_BUS, RX_SIZE_256, TX_SIZE_16> can_bus;

void canBegin() {
    can_bus.begin();
    can_bus.setBaudRate(100000);
}

void canSend(uint32_t id, const uint8_t* data) {
    CAN_message_t msg;
    msg.id = id;
    msg.len = 8;
    memcpy(msg.buf, data, 8);
    can_bus.write(msg);
}

void canPoll(const CanHandlerEntry* handlers, size_t count) {
    CAN_message_t msg;
    while (can_bus.read(msg)) {
        for (size_t i = 0; i < count; ++i) {
            if (msg.id == handlers[i].id) {
                handlers[i].handler(msg.buf);
                break;
            }
        }
    }
}

#endif
