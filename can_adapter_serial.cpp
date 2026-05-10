#include "config.h"

#if !defined(USE_MCP_CAN_SPI) && !defined(USE_ESP32_TWAI)

#include <Arduino.h>
#include <string.h>
#include "can_adapter.h"

// Longan Serial CAN bus adapter: https://docs.longan-labs.cc/1030001/
#define canSerial Serial1

#define FRAME_SIZE 14
static uint8_t buffer[FRAME_SIZE];

void canBegin() {
    canSerial.begin(CAN_SERIAL_BAUD);

#if defined(__AVR_AT90USB1286__)
    // Pick the slightly slower baudrate as the higher more accurate one does not work
    // with the CAN bus adapter
    UCSR1A &= ~(1 << U2X1);
    UBRR1 = 8;
#endif
}

void canSend(uint32_t id, const uint8_t* data) {
    uint8_t buf[FRAME_SIZE] = {
        (uint8_t)((id >> 24) & 0xFF), (uint8_t)((id >> 16) & 0xFF),
        (uint8_t)((id >> 8) & 0xFF), (uint8_t)(id & 0xFF),
        0x00, 0x00
    };
    memcpy(&buf[6], data, 8);
    canSerial.write(buf, FRAME_SIZE);
}

static bool match_id(const uint8_t* buf, uint32_t id) {
    return
        buf[0] == ((id >> 24) & 0xFF) &&
        buf[1] == ((id >> 16) & 0xFF) &&
        buf[2] == ((id >> 8) & 0xFF) &&
        buf[3] == (id & 0xFF);
}

void canPoll(const CanHandlerEntry* handlers, size_t count) {
    while (canSerial.available()) {
        for (int i = 0; i < FRAME_SIZE - 1; ++i) {
            buffer[i] = buffer[i + 1];
        }
        buffer[FRAME_SIZE - 1] = canSerial.read();

        for (size_t i = 0; i < count; ++i) {
            if (match_id(buffer, handlers[i].id)) {
                handlers[i].handler(buffer + 4);
                break;
            }
        }
    }
}

#endif
