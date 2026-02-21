#include <Arduino.h>
#include "types.h"
#include "serial.h"
#include "config.h"
#include "pc_printf.h"

extern SInput s_input;

#define FRAME_LENGTH 35
char rx_buf[FRAME_LENGTH];
size_t rx_pos = 0;
bool line_ready = false;

void serialRead() {
    while (pc.available()) {
        char c = pc.read();
        if (rx_pos == 0 && c != 'S') {
            // Waiting for the start character but received something else so ignore it
        } else if (rx_pos == FRAME_LENGTH - 1) {
            rx_buf[rx_pos] = c;
            line_ready = true;
            rx_pos = 0;
        } else {
            rx_buf[rx_pos++] = c;
        }
    }
}

static inline uint16_t parse_u16(const uint8_t* p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static inline uint32_t parse_u32(const uint8_t* p) {
    return (uint32_t)p[0]
         | ((uint32_t)p[1] << 8)
         | ((uint32_t)p[2] << 16)
         | ((uint32_t)p[3] << 24);
}

void serialParse() {
      digitalWrite(LED_BUILTIN, 0);

    if (!line_ready) return;
    line_ready = false;

    const uint8_t payloadLength = FRAME_LENGTH - 2;
    const uint8_t* p = (const uint8_t*)rx_buf;

    if (p[0] != 'S') {
        serial_printf(pc, "[UART] Invalid frame marker\n");
        return;
    }

    uint8_t checksumReceived = p[payloadLength + 1];
    uint8_t checksumCalculated = 0;

    for (int i = 1; i < payloadLength + 1; i++) {
        checksumCalculated = (checksumCalculated + p[i]) & 0xFF;
    }

    if (checksumCalculated != checksumReceived) {
        serial_printf(pc, "[UART] Checksum mismatch: received %02X, calculated %02X\n", checksumReceived, checksumCalculated);
        return;
    }

    int idx = 1; // skip 'S'

    // Timestamp
    s_input.time_year   = p[idx++] + 2000;
    s_input.time_month  = p[idx++];
    s_input.time_day    = p[idx++];
    s_input.time_hour   = p[idx++];
    s_input.time_minute = p[idx++];
    s_input.time_second = p[idx++];

    s_input.rpm         = parse_u16(&p[idx]); idx += 2;
    s_input.speed       = parse_u16(&p[idx]); idx += 2;

    uint8_t gear        = p[idx++];
    s_input.water_temp  = p[idx++];
    s_input.oil_temp    = p[idx++];
    s_input.fuel        = parse_u16(&p[idx]); idx += 2;

    uint32_t flags      = parse_u32(&p[idx]); idx += 4;

    // Parse flags using bitmasks
    s_input.light_shift      = flags & (1UL << 0);
    s_input.light_highbeam   = flags & (1UL << 1);
    s_input.handbrake        = flags & (1UL << 2);
    s_input.light_tc_active  = flags & (1UL << 4);

    bool left_signal  = flags & (1UL << 5);
    bool right_signal = flags & (1UL << 6);

    if (left_signal && right_signal)
        s_input.indicator_state = I_HAZZARD;
    else if (left_signal)
        s_input.indicator_state = I_LEFT;
    else if (right_signal)
        s_input.indicator_state = I_RIGHT;
    else
        s_input.indicator_state = I_OFF;

    s_input.oil_warn           = flags & (1UL << 8);
    s_input.battery_warn       = flags & (1UL << 9);
    s_input.abs_warn           = flags & (1UL << 10);
    s_input.light_beacon       = flags & (1UL << 11);
    s_input.light_lowbeam      = flags & (1UL << 12);
    s_input.light_esc_active   = flags & (1UL << 13);
    s_input.check_engine       = flags & (1UL << 14);
    s_input.clutch_temp        = flags & (1UL << 15);
    s_input.light_fog          = flags & (1UL << 16);
    s_input.brake_temp         = flags & (1UL << 17);

    s_input.tires.fl_deflated = flags & (1UL << 18);
    s_input.tires.fr_deflated = flags & (1UL << 19);
    s_input.tires.rl_deflated = flags & (1UL << 20);
    s_input.tires.rr_deflated = flags & (1UL << 21);

    bool all_deflated = s_input.tires.fl_deflated &&
                        s_input.tires.fr_deflated &&
                        s_input.tires.rl_deflated &&
                        s_input.tires.rr_deflated;

    s_input.tires.all_deflated = all_deflated;
    if (all_deflated) {
        s_input.tires.fl_deflated = false;
        s_input.tires.fr_deflated = false;
        s_input.tires.rl_deflated = false;
        s_input.tires.rr_deflated = false;
    }

    s_input.radiator_warn      = flags & (1UL << 22);
    s_input.engine_temp_yellow = flags & (1UL << 23);
    s_input.engine_temp_red    = flags & (1UL << 24);

    s_input.doors.fl_open = flags & (1UL << 25);
    s_input.doors.fr_open = flags & (1UL << 26);
    s_input.doors.rl_open = flags & (1UL << 27);
    s_input.doors.rr_open = flags & (1UL << 28);
    s_input.doors.tailgate_open = flags & (1UL << 29);

    s_input.light_tc_disabled  = flags & (1UL << 30);
    s_input.light_esc_disabled = flags & (1UL << 31);

    uint8_t flagsExt = p[idx++];
    s_input.yellow_triangle  = flagsExt & (1UL << 0);
    s_input.red_triangle     = flagsExt & (1UL << 1);
    s_input.gear_issue       = flagsExt & (1UL << 2);
    s_input.exclamation_mark = flagsExt & (1UL << 3);
    s_input.adblue_low       = flagsExt & (1UL << 4);

    s_input.fuel_injection   = parse_u16(&p[idx]); idx += 2;
    s_input.custom_light     = parse_u16(&p[idx]); idx += 2;
    s_input.custom_light_on  = p[idx++] != 0;
    uint8_t gearMode         = p[idx++];
    s_input.cruise.speed     = parse_u16(&p[idx]); idx += 2;

    uint8_t cruiseMode       = p[idx++];
    s_input.cruise.enabled   = cruiseMode & 0x01;
    s_input.cruise.acc.yellow_car_static = (cruiseMode & 0x02) != 0;
    s_input.cruise.acc.red_car_blinking = (cruiseMode & 0x04) != 0;
    uint8_t distanceCode = (cruiseMode >> 3) & 0x07;
    s_input.cruise.acc.distance = (distanceCode >= 1 && distanceCode <= 4) ? distanceCode : 0;

    s_input.ignition         = (IGNITION_STATE)p[idx++];
    s_input.engine_running   = p[idx++] != 0;

    s_input.ambient_temp = parse_u16(&p[idx]); idx += 2;

    // Gear logic
    if (gearMode == 'P') {
        s_input.explicitGear = NONE;
        s_input.currentGear = PARK;
        s_input.mode = NORMAL;
    } else if (gear == NEUTRAL) {
        s_input.explicitGear = NONE;
        s_input.currentGear = NEUTRAL;
        s_input.mode = NORMAL;
    } else if (gear == REVERSE) {
        s_input.explicitGear = NONE;
        s_input.currentGear = REVERSE;
        s_input.mode = NORMAL;
    } else if (gearMode == 'A') {
        s_input.explicitGear = NONE;
        s_input.currentGear = DRIVE;
        s_input.mode = NORMAL;
    } else {
        s_input.explicitGear = (GEAR_MANUAL)(min(gear - 1, NUMBER_OF_GEARS));
        s_input.currentGear = DRIVE;
        s_input.mode = (gearMode == 'S') ? SPORT : NORMAL;
    }

    digitalWrite(LED_BUILTIN, 1);
}
