#include "config.h"

#if defined(USE_ESP32_TWAI)

#include <Arduino.h>
#include <string.h>
#include "driver/twai.h"
#include "can_adapter.h"
#include "serial.h"

void canBegin() {
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(
        (gpio_num_t)TWAI_TX_PIN,
        (gpio_num_t)TWAI_RX_PIN,
        TWAI_MODE_NO_ACK
    );
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_100KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    while (twai_driver_install(&g_config, &t_config, &f_config) != ESP_OK
           || twai_start() != ESP_OK) {
        pc.println("CAN BUS init fail, retrying...");
        twai_stop();
        twai_driver_uninstall();
        delay(100);
    }
}

void canSend(uint32_t id, const uint8_t* data) {
    twai_message_t msg = {};
    msg.identifier = id;
    msg.data_length_code = 8;
    memcpy(msg.data, data, 8);
    twai_transmit(&msg, pdMS_TO_TICKS(2));
}

void canPoll(const CanHandlerEntry* handlers, size_t count) {
    twai_message_t msg;
    while (twai_receive(&msg, 0) == ESP_OK) {
        for (size_t i = 0; i < count; ++i) {
            if (msg.identifier == handlers[i].id) {
                handlers[i].handler(msg.data);
                break;
            }
        }
    }
}

#endif
