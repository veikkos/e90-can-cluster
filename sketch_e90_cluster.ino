#include <Arduino.h>
#include <math.h>

// Config defines
#define READ_FRAMES_FROM_CLUSTER
//#define READ_FRAMES_FROM_CLUSTER_1B4
#define READ_FRAMES_FROM_CLUSTER_330
//#define READ_FRAMES_FROM_CLUSTER_2C0

// Serial interfaces
#define canSerial Serial1
#define pc Serial

// Timers
uint32_t lastTime = 0;
uint32_t lastTaskTime = 0;
uint16_t canCounter = 0;

// Refueling led
#define REFUELING_LED_PIN 45

// Define M_PI if not present
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Enums
enum INDICATOR {
    I_OFF = 0,
    I_LEFT,
    I_RIGHT,
    I_HAZZARD
};

enum CAN_LIGHTS {
    L_BRAKE = 0x80,
    L_FOG = 0x40,
    L_BACKLIGHT = 0x04,
    L_MAIN = 0x02,
    L_DIP = 0x20
};

enum GEAR {
    PARK = -1,
    REVERSE = 0,
    NEUTRAL = 1,
    DRIVE = 2
};

enum GEAR_MANUAL {
    NONE = 0, M1, M2, M3, M4, M5, M6
};

enum GEAR_MODE {
    NORMAL,
    SPORT
};

enum IGNITION_STATE {
    IG_OFF = 0,
    IG_ACCESSORY = 1,
    IG_ON = 2,
    IG_STARTER = 3
};

enum ErrorLightID : uint16_t {
    BOOT_OPEN = 19,
    FIX_CAR = 21,
    YELLOW_WARNING = 24,
    OIL_LEVEL_LOW = 28,
    CHECK_ENGINE_DOUBLE = 31,
    CHECK_ENGINE = 34,
    DTC_TRIANGLE_DOUBLE_EXCLAMATION_MARK = 35,
    YELLOW_TRIANGLE = 37,
    OVERHEAT_RED = 39,
    START_FOOT_BRAKE = 40,
    RED_TRIANGLE = 41,
    ABS_DTC_TRIANGLE_WARNING = 42,
    SUSPENSION_WARNING = 43,
    PARTICLE_FILTER = 49,
    TIRE_PRESSURE_YELLOW = 50,
    LIMIT_RED = 62,
    TIRE_PRESSURE_RED = 63,
    CRUISE_WARNING = 69, // Also 85
    STEERING_WARNING = 73,
    LIMIT_YELLOW = 78,
    COLD_WEATHER = 79,
    BATTERY_YELLOW = 161,
    EXCLAMATION_MARK_YELLOW = 169,
    DTC_DOUBLE_BLINKING = 184,
    LOW_TIRE_PRESSURE_FRONT_LEFT = 139,
    LOW_TIRE_PRESSURE_REAR_RIGHT = 140,
    LOW_TIRE_PRESSURE_REAR_LEFT = 141,
    LOW_TIRE_PRESSURE_FRONT_RIGHT = 143,
    LOW_TIRE_PRESSURE_ALL = 147,
    DSC_TRIANGLE_DOUBLE = 36,
    DSC_TRIANGLE_SYMBOL_ONLY = 215,
    FOOT_TO_BRAKE = 244,
    GEAR_ISSUE = 348,
    OIL_RED = 212,
    BATTERY_RED = 213,
    SOS_ERROR = 299,
    OVERHEAT_YELLOW = 257,
    FUEL_WARNING = 275,
    SERVICE_LIGHT = 281,
    CAR_AHEAD_YELLOW = 282,
    CAR_AHEAD_RED = 283,
    AUTO_GEAR_BOX_YELLOW = 349,
    DTC_SYMBOL_ONLY = 357,
    GEARBOX_TEMP_YELLOW = 104,
    GEARBOX_TEMP_RED = 105,
    BRAKES_HOT = 353,
    COOLANT_LOW = 166,
    CAR_CAN_ROLL = 302,
    DOOR_OPEN_LEFT = 715,
    DOOR_OPEN_RIGHT = 716,
    DSC_OFF = 673,
    ALARM_LIGHT_EXCLAMATION = 162,
    ALARM_LIGHT = 508
    // What's there on 400 and beyond...?
};

// Vehicle state structure
struct SInput {
    IGNITION_STATE ignition = IG_ON;
    INDICATOR indicator_state = I_OFF;

    uint16_t time_year = 2010;
    uint8_t time_month = 1;
    uint8_t time_day = 1;
    uint8_t time_hour = 12;
    uint8_t time_minute = 0;
    uint8_t time_second = 0;

    uint16_t rpm = 0;
    uint16_t speed = 0;
    GEAR currentGear = PARK;
    GEAR_MANUAL manualGear = NONE;
    GEAR_MODE mode = SPORT;
    uint16_t fuel = 1000;
    uint16_t fuel_injection = 0;
    uint8_t water_temp = 0;
    uint16_t custom_light = 0;

    bool custom_light_on = false;
    bool light_shift = false;
    bool light_highbeam = false;
    bool light_lowbeam = true;
    bool light_fog = false;
    bool light_tc_active = false;
    bool light_esc_active = false;
    bool light_tc_disabled = false;
    bool light_esc_disabled = false;
    bool light_beacon = false;
    bool oil_warn = false;
    bool battery_warn = false;
    bool abs_warn = false;
    bool engine_temp_yellow = false;
    bool engine_temp_red = false;
    bool check_engine = false;
    bool clutch_temp = false;
    bool brake_temp = false;
    bool radiator_warn = false;
    bool engine_running = true;

    struct {
        bool fl_deflated = false;
        bool fr_deflated = false;
        bool rl_deflated = false;
        bool rr_deflated = false;
        bool all_deflated = false;
    } tires;

    struct {
        bool fl_open = false;
        bool fr_open = false;
        bool rl_open = false;
        bool rr_open = false;
        bool tailgate_open = false;
    } doors;

    bool handbrake = false;

    bool cruise_enabled = false;
    uint16_t cruise_speed = 0;
};

SInput s_input;

// Serial RX buffer
#define FRAME_LENGTH 31
char rx_buf[FRAME_LENGTH];
size_t rx_pos = 0;
bool line_ready = false;

void readSerial() {
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

void parseTelemetryLine() {
    digitalWrite(LED_BUILTIN, 0);

    if (!line_ready) return;
    line_ready = false;

    const uint8_t payloadLength = FRAME_LENGTH - 2;
    const uint8_t* p = (const uint8_t*)rx_buf;

    if (p[0] != 'S') {
        pc.printf("[UART] Invalid frame marker\n");
        return;
    }

    uint8_t checksumReceived = p[payloadLength + 1];
    uint8_t checksumCalculated = 0;

    for (int i = 1; i < payloadLength + 1; i++) {
        checksumCalculated = (checksumCalculated + p[i]) & 0xFF;
    }

    if (checksumCalculated != checksumReceived) {
        pc.printf("[UART] Checksum mismatch: received %02X, calculated %02X\n", checksumReceived, checksumCalculated);
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

    s_input.fuel_injection   = parse_u16(&p[idx]); idx += 2;
    s_input.custom_light     = parse_u16(&p[idx]); idx += 2;
    s_input.custom_light_on  = p[idx++] != 0;
    uint8_t gearMode         = p[idx++];
    s_input.cruise_speed     = parse_u16(&p[idx]) / 10; idx += 2;
    s_input.cruise_enabled   = p[idx++] != 0;

    s_input.ignition         = (IGNITION_STATE)p[idx++];
    s_input.engine_running   = p[idx++] != 0;

    // Gear logic
    if (gearMode == 'P') {
        s_input.manualGear = NONE;
        s_input.currentGear = PARK;
        s_input.mode = NORMAL;
    } else if (gear == NEUTRAL) {
        s_input.manualGear = NONE;
        s_input.currentGear = NEUTRAL;
        s_input.mode = NORMAL;
    } else if (gear == REVERSE) {
        s_input.manualGear = NONE;
        s_input.currentGear = REVERSE;
        s_input.mode = NORMAL;
    } else if (gearMode == 'A') {
        s_input.manualGear = NONE;
        s_input.currentGear = DRIVE;
        s_input.mode = NORMAL;
    } else {
        s_input.manualGear = (GEAR_MANUAL)(min(gear - 1, 6));
        s_input.currentGear = DRIVE;
        s_input.mode = (gearMode == 'S') ? SPORT : NORMAL;
    }

    digitalWrite(LED_BUILTIN, 1);
}

void sendCAN(uint32_t id, const uint8_t* data) {
    uint8_t buf[14] = {
        (uint8_t)((id >> 24) & 0xFF), (uint8_t)((id >> 16) & 0xFF),
        (uint8_t)((id >> 8) & 0xFF), (uint8_t)(id & 0xFF),
        0x00, 0x00
    };
    memcpy(&buf[6], data, 8);
    canSerial.write(buf, 14);
}

void canSendIgnitionFrame() {
    const uint32_t ID = 0x130;
    static uint8_t counter_on = 0xE2;
    static uint8_t byte0 = 0x00;

    if (s_input.engine_running) {
        byte0 = 0x45;
    } else if (s_input.ignition >= IG_ON) {
       byte0 = 0x55;
    } else if (s_input.ignition == IG_ACCESSORY) {
        byte0 = 0x41;
    } else {
        byte0 = 0x00;
    }

    uint8_t data[8] = {byte0, 0x42, 0x69, 0x8F, counter_on++, 0, 0, 0};
    sendCAN(ID, data);
}

void canSendRPM() {
    const uint32_t ID = 0x00AA;
    uint16_t rpm_val = min(s_input.rpm, (uint16_t)7500) * 4;
    uint8_t data[8] = {0x5F, 0x59, 0xFF, 0x00,
                       (uint8_t)(rpm_val & 0xFF), (uint8_t)(rpm_val >> 8),
                       0x80, 0x99};
    sendCAN(ID, data);
}

inline uint16_t kmhx10ToMph(uint16_t kmh_x10, uint32_t correction) {
    return (min((uint32_t)kmh_x10, 2600) * (620 + correction) + 5000) / 10000;
}

void canSendSpeed() {
    const uint32_t ID = 0x1A6;
    static uint16_t last_speed_counter = 0;
    static uint16_t last_tick_counter = 0;
    uint32_t delta_ms = 100;
    const int calibration = 30; // This value is empirically set so the speed matches on this particular cluster
    uint16_t speed_mph = kmhx10ToMph(s_input.speed, calibration);
    uint16_t current_speed_counter = speed_mph + last_speed_counter;
    uint16_t delta_tick_counter = delta_ms * 2;
    uint16_t tick_counter = last_tick_counter + delta_tick_counter;

    uint8_t low = current_speed_counter & 0xFF;
    uint8_t high = current_speed_counter >> 8;

    uint8_t frame[8] = {
        low, high,
        low, high,
        low, high,
        (uint8_t)(tick_counter & 0xFF),
        (uint8_t)(0xF0 | ((tick_counter >> 8) & 0x0F))
    };

    sendCAN(ID, frame);
    last_speed_counter = current_speed_counter;
    last_tick_counter = tick_counter;
}

void canSendLights() {
    const uint32_t ID = 0x21A;
    static uint8_t frame[8] = {0x00, 0x00, 0xF7, 0, 0, 0, 0, 0};

    uint8_t lights = 0;
    if (s_input.light_lowbeam || s_input.light_highbeam) lights |= L_BACKLIGHT;
    if (s_input.light_fog) lights |= L_DIP;
    if (s_input.light_highbeam) lights |= L_MAIN;
    if (s_input.light_fog) lights |= L_FOG;

    frame[0] = lights;
    sendCAN(ID, frame);
}

void canSendIndicator() {
    const uint32_t ID = 0x1F6;
    uint8_t frame[8] = {0x80, 0xF0, 0, 0, 0, 0, 0, 0};

    switch (s_input.indicator_state) {
        case I_LEFT:    frame[0] = 0x91; break;
        case I_RIGHT:   frame[0] = 0xA1; break;
        case I_HAZZARD: frame[0] = 0xB1; break;
        case I_OFF:
        default:        frame[0] = 0x80; break;
    }
    sendCAN(ID, frame);
}

void canSendSteeringWheel() {
    const uint32_t ID = 0x0C4;
    static uint8_t frame[8] = {0x83, 0xFD, 0xFC, 0x00, 0x00, 0xFF, 0xF1, 0x00};
    frame[1] = 0; frame[2] = 0;
    sendCAN(ID, frame);
}

void canSendAbs() {
    const uint32_t ID = 0x19E;
    static uint8_t counter = 0;

    uint8_t frame[8] = {
        0x00, 0xE0, 0x00, 0xFC, 0xCC, 0x00, 0x00, 0x00
    };

    // Byte 2 rotation (high nibble cycles, low nibble is always 0x03)
    uint8_t rotate_nibble = (counter % 3) << 4;
    frame[2] = rotate_nibble | 0x03;
    frame[5] = 0x00;
    frame[6] = 0x00;
    frame[7] = counter++;

    sendCAN(ID, frame);
}

void canSendEngineTempAndFuelInjection() {
    const uint32_t ID = 0x1D0;
    static uint8_t frame[8] = {0x8B, 0xFF, 0x00, 0xCD, 0x00, 0x00, 0xCD, 0xA8};
    const uint8_t engine_run_state = s_input.engine_running ? 0x2 : 0x0;  // 0x0 = off, 0x1 = starting, 0x2 = running, 0x3 = invalid
    static uint16_t fuel_injection_total = 0;

    frame[0] = s_input.water_temp + 48;

    // Update alive counter (lower 4 bits)
    static uint8_t alive_counter = 0;
    alive_counter = (alive_counter + 1) & 0x0F;

    // Encode engine_run_state into bits 4–5, preserve upper/lower bits
    frame[2] = alive_counter;
    frame[2] |= (engine_run_state & 0x03) << 4; // bits 4–5

    // Value is cumulative fuel injected in 100 ms cycles
    fuel_injection_total += s_input.fuel_injection;

    frame[4] = fuel_injection_total & 0xFF;
    frame[5] = (fuel_injection_total >> 8) & 0xFF;

    sendCAN(ID, frame);
}

void canSendAbsCounter() {
    const uint32_t ID = 0x0C0;
    static uint8_t frame[8] = {0xF0, 0xFF, 0, 0, 0, 0, 0, 0};
    sendCAN(ID, frame);
    frame[0] = ((frame[0] + 1) | 0xF0);
}

void canSendAirbagCounter() {
    const uint32_t ID = 0x0D7;
    static uint8_t frame[8] = {0xC3, 0xFF, 0, 0, 0, 0, 0, 0};
    sendCAN(ID, frame);
    frame[0]++;
}

// This function is not fully verified but the presence of it makes the average speed and average fuel consumption work. See
// https://github.com/HeinrichG-V12/E65_ReverseEngineering/blob/main/docs/0x1A0.md
void canSendVehicleDynamics() {
    const uint32_t ID = 0x1A0;
    static uint8_t alive_counter = 0;

    float speed_kmh = s_input.speed / 10.f;
    // Moving forward, backward not supported yet
    uint8_t st_veh_dvco = speed_kmh >= 1 ? 1 : 0;
    float acc_long = 0.f;       // m/s²
    float acc_lat  = 0.f;       // m/s²
    float yaw_rate = 0.f;       // deg/s

    uint16_t v_veh_raw = (uint16_t)(speed_kmh / 0.1f);
    int16_t acc_long_raw = (int16_t)(acc_long / 0.025f);
    int16_t acc_lat_raw  = (int16_t)(acc_lat  / 0.025f);
    int16_t yaw_rate_raw = (int16_t)(yaw_rate / 0.05f);

    uint8_t frame[8] = {0};

    frame[0] = v_veh_raw & 0xFF;
    frame[1] = (st_veh_dvco & 0x07) << 4;
    frame[2] = acc_long_raw & 0xFF;
    frame[3] = ((acc_long_raw >> 8) & 0x0F) | ((acc_lat_raw & 0x0F) << 4);
    frame[4] = (acc_lat_raw >> 4) & 0xFF;
    frame[5] = yaw_rate_raw & 0xFF;
    frame[6] = ((yaw_rate_raw >> 8) & 0x0F) | ((alive_counter++ & 0x0F) << 4);

    uint8_t checksum = 0x00;
    for (int i = 0; i < 7; i++) {
        checksum ^= frame[i];
    }
    frame[7] = checksum;

    sendCAN(ID, frame);
}

struct FuelLevelPoint {
    float fuel_percent;  // 1.0 = 100%, 0.75 = 75%, etc.
    uint16_t meter_level; // Value to send to the cluster
};

FuelLevelPoint fuelTableLeft[] = {
    {1.00f, 9500},
    {0.875f, 8200},
    {0.75f, 6250},
    {0.50f, 3600},
    {0.25f, 1950},
    {0.00f,  650}
};

FuelLevelPoint fuelTableRight[] = {
    {1.00f, 8000},
    {0.875f, 5400},
    {0.75f, 4600},
    {0.50f, 3350},
    {0.25f, 2200},
    {0.00f, 1000}
};

// This is bit heavy floating point calculation but fine for now
uint16_t interpolateFuel(float percent, struct FuelLevelPoint table[5]) {
    for (int i = 0; i < 4; ++i) {
        if (percent >= table[i + 1].fuel_percent) {
            float range = table[i].fuel_percent - table[i + 1].fuel_percent;
            float alpha = (percent - table[i + 1].fuel_percent) / range;
            return (uint16_t)(table[i + 1].meter_level +
                              alpha * (table[i].meter_level - table[i + 1].meter_level));
        }
    }

    return table[4].meter_level; // fallback for <0%
}

inline float currentFuelToFloat() {
    return (float)s_input.fuel / 1000.f;
}

void canSendFuel() {
    const uint32_t ID = 0x349;
    static uint8_t frame[8] = {0};
    static float previousFuel = currentFuelToFloat();

    const float maxDeltaRatePerSecond = 0.015f;
    const float callIntervalSeconds = 0.2f;
    const float maxDelta = maxDeltaRatePerSecond * callIntervalSeconds;

    float fuel = currentFuelToFloat();
    if (fuel > 1.0f) fuel = 1.0f;
    if (fuel < 0.0f) fuel = 0.0f;

    float delta = fuel - previousFuel;
    bool refueling = false;

    if (delta > maxDelta) {
        fuel = previousFuel + maxDelta;
        refueling = true;
    } else if (delta < -maxDelta) {
        fuel = previousFuel - maxDelta;
        refueling = true;
    }

    previousFuel = fuel;
    digitalWrite(REFUELING_LED_PIN, refueling ? HIGH : LOW);

    // Fuel gauge is not linear so match it here
    uint16_t levelLeft = interpolateFuel(fuel, fuelTableLeft);

    frame[0] = levelLeft & 0xFF;
    frame[1] = (levelLeft >> 8);

    // There are two sensors
    uint16_t levelRight = interpolateFuel(fuel, fuelTableRight);

    frame[2] = levelRight & 0xFF;
    frame[3] = (levelRight >> 8);

    sendCAN(ID, frame);
}

void canSendHandbrake() {
    const uint32_t ID = 0x34F;
    static uint8_t frame[8] = {0xFE, 0xFF, 0, 0, 0, 0, 0, 0};
    frame[0] = s_input.handbrake ? 0xFE : 0xFD;
    sendCAN(ID, frame);
}

void canSendTime() {
    const uint32_t ID = 0x39E;
    uint8_t data[8] = {
        s_input.time_hour,
        s_input.time_minute,
        s_input.time_second,
        s_input.time_day,
        (uint8_t)((s_input.time_month << 4) | 0x0F),
        (uint8_t)s_input.time_year,
        (uint8_t)(s_input.time_year >> 8),
        0xF2
    };
    sendCAN(ID, data);
}

void canSendDmeStatus() {
    const uint32_t ID = 0x12F;
    uint8_t frame[8] = {0x3F, 0x00, 0x00, 0x00, 0x00, 0x40, 0x01, 0x00};
    sendCAN(ID, frame);
}

void canSendErrorLight(uint16_t light_id, bool enable) {
    const uint32_t ID = 0x592;
    const uint8_t ON = 0x31;
    const uint8_t OFF = 0x30;

    uint8_t frame[] = {
        0x40,
        static_cast<uint8_t>(light_id & 0xFF),
        static_cast<uint8_t>((light_id >> 8) & 0xFF),
        enable ? ON : OFF,
        0xFF, 0xFF, 0xFF, 0xFF
    };
    sendCAN(ID, frame);
}

void canSuppressService() {
    canSendErrorLight(SERVICE_LIGHT, false);
}

void canSuppressSos() {
    const uint32_t ID = 0x0C1;
    uint8_t frame[8] = { (uint8_t)rand(), 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    sendCAN(ID, frame);
}

void canSendGearboxData() {
    const uint32_t ID = 0x1D2;

    // Byte 0 – Automatic gear
    uint8_t byte0 = 0x00;
    switch (s_input.currentGear) {
        case PARK:    byte0 = 0xE1; break;
        case REVERSE: byte0 = 0xD2; break;
        case NEUTRAL: byte0 = 0xB4; break;
        case DRIVE:   byte0 = 0x78; break;
    }

    // Byte 1 – Manual gear selection or 0x0F if in automatic
    uint8_t byte1 = 0x0F;
    if (s_input.manualGear != NONE) {
        switch (s_input.manualGear) {
            case M1: byte1 = 0x5F; break;
            case M2: byte1 = 0x6F; break;
            case M3: byte1 = 0x7F; break;
            case M4: byte1 = 0x8F; break;
            case M5: byte1 = 0x9F; break;
            case M6: byte1 = 0xAF; break;
            default: byte1 = 0x0F; break;
        }
    }

    // Byte 2 – Always 0xFF
    uint8_t byte2 = 0xFF;

    // Byte 3 – Counter with high nibble cycling, low nibble fixed
    static uint8_t counter_high = 0;
    uint8_t byte3;

    if (s_input.currentGear == PARK || s_input.currentGear == REVERSE) {
        byte3 = (counter_high << 4) | 0x0C; // 0x0C to 0xFC
        counter_high = (counter_high + 1) % 16;
    } else {
        // 0x0F shows "Sport" on the cluster
        byte3 = (counter_high << 4) | (s_input.mode == NORMAL ? 0x0D : 0x07);
        counter_high = (counter_high + 1) % 15;
    }

    // Byte 4 – Drive mode (F0 = Drive, F2 = Drive Sport)
    uint8_t byte4 = s_input.manualGear != NONE ? 0xF2 : 0xF0;

    // Byte 5 – Always 0xFF
    uint8_t byte5 = 0xFF;

    uint8_t frame[8] = {
        byte0, byte1, byte2, byte3, byte4, byte5, 0xFF, 0xFF
    };

    sendCAN(ID, frame);
}

#define DEFINE_CAN_SEND_SYMBOL(fn_name, signal_expr, symbol_const, interval, offset) \
    void fn_name() {                                                                 \
        static bool last_value = false;                                              \
        static uint8_t counter = offset;                                             \
        bool value = (signal_expr) && (s_input.ignition >= IG_ON);                   \
        counter++;                                                                   \
        if (value != last_value || (counter % (interval) == 0)) {                    \
            last_value = value;                                                      \
            counter = 0;                                                             \
            canSendErrorLight(symbol_const, value);                                  \
        }                                                                            \
    }

bool shouldShowDoorOpenLeftWarning() {
    return s_input.currentGear != PARK && (
        s_input.doors.fl_open ||
        s_input.doors.rl_open
    );
}

bool shouldShowDoorOpenRightWarning() {
    return s_input.currentGear != PARK && (
        s_input.doors.fr_open ||
        s_input.doors.rr_open
    );
}

// Interval = 200 ms
DEFINE_CAN_SEND_SYMBOL(canSendEngineTempYellowSymbol, s_input.engine_temp_yellow && !s_input.engine_temp_red, OVERHEAT_YELLOW, 25, 0)
DEFINE_CAN_SEND_SYMBOL(canSendEngineTempRedSymbol, s_input.engine_temp_red, OVERHEAT_RED, 25, 1)
DEFINE_CAN_SEND_SYMBOL(canSendCheckEngineSymbol, s_input.check_engine, CHECK_ENGINE_DOUBLE, 25, 2)
DEFINE_CAN_SEND_SYMBOL(canSendClutchTempSymbol, s_input.clutch_temp, GEARBOX_TEMP_YELLOW, 25, 3)
DEFINE_CAN_SEND_SYMBOL(canSendOilWarningSymbol, s_input.oil_warn, OIL_RED, 25, 4)
DEFINE_CAN_SEND_SYMBOL(canSendBatteryWarningSymbol, s_input.battery_warn , BATTERY_RED, 25, 5)
DEFINE_CAN_SEND_SYMBOL(canSendBrakeTempSymbol, s_input.brake_temp, BRAKES_HOT, 25, 6)
DEFINE_CAN_SEND_SYMBOL(canSendTireDeflatedFl, s_input.tires.fl_deflated, LOW_TIRE_PRESSURE_FRONT_LEFT, 25, 7)
DEFINE_CAN_SEND_SYMBOL(canSendTireDeflatedFr, s_input.tires.fr_deflated, LOW_TIRE_PRESSURE_FRONT_RIGHT, 25, 8)
DEFINE_CAN_SEND_SYMBOL(canSendTireDeflatedRl, s_input.tires.rl_deflated, LOW_TIRE_PRESSURE_REAR_LEFT, 25, 9)
DEFINE_CAN_SEND_SYMBOL(canSendTireDeflatedRr, s_input.tires.rr_deflated, LOW_TIRE_PRESSURE_REAR_RIGHT, 25, 10)
DEFINE_CAN_SEND_SYMBOL(canSendTireDeflatedAll, s_input.tires.all_deflated, LOW_TIRE_PRESSURE_ALL, 25, 11)
DEFINE_CAN_SEND_SYMBOL(canSendRadiatorSymbol, s_input.radiator_warn, COOLANT_LOW, 25, 12)
DEFINE_CAN_SEND_SYMBOL(canSendDoorOpenLeft, shouldShowDoorOpenLeftWarning(), DOOR_OPEN_LEFT, 25, 13)
DEFINE_CAN_SEND_SYMBOL(canSendDoorOpenRight, shouldShowDoorOpenRightWarning(), DOOR_OPEN_RIGHT, 25, 14)
DEFINE_CAN_SEND_SYMBOL(canSendTailgateOpen, s_input.doors.tailgate_open, BOOT_OPEN, 25, 15)
DEFINE_CAN_SEND_SYMBOL(canSendEscDisabledSymbol, s_input.light_esc_disabled, DSC_TRIANGLE_DOUBLE, 25, 16)
DEFINE_CAN_SEND_SYMBOL(canSendBeaconSymbol, s_input.light_beacon, ALARM_LIGHT, 25, 17)

// Interval = 50 ms
DEFINE_CAN_SEND_SYMBOL(canSendTcSymbol, s_input.light_tc_active || s_input.light_tc_disabled, DTC_SYMBOL_ONLY, 100, 1)
DEFINE_CAN_SEND_SYMBOL(canSendEscSymbol, s_input.light_esc_active, DSC_TRIANGLE_SYMBOL_ONLY, 100, 2)

void canSendCustomSymbol() {
    static bool last_state = false;
    static uint16_t last_number = 0;
    static uint8_t counter = 0;
    const bool changed = s_input.custom_light != last_number || last_state != s_input.custom_light_on;

    if (changed || (++counter % 10 == 0)) {
        last_state = s_input.custom_light_on;
        last_number = s_input.custom_light;
        counter = 0;
        canSendErrorLight(last_number, last_state);
    }
}

void canSendOilLevel() {
    const uint32_t ID = 0x381;
    // min: 0x19
    // middle: 0x35
    const uint8_t level = 0x35;
    // >MAX: e.g. 0xF1
    // OK: 0xF0
    // +1l: 0xF2
    const uint8_t status = 0xF0;
    static uint8_t frame[8] = {level, status, 0xFF, 0, 0, 0, 0, 0};
    sendCAN(ID, frame);
}

inline uint8_t getCruiseTimer(uint16_t call_interval_ms = 100) {
    static uint8_t timer = 0;
    static uint16_t ms_accumulator = 0;

    const uint16_t TIMER_INTERVAL_MS = 200;
    const uint8_t TIMER_STEP = 17;

    ms_accumulator += call_interval_ms;
    if (ms_accumulator >= TIMER_INTERVAL_MS) {
        ms_accumulator = 0;
        timer = (timer + TIMER_STEP) % 256;
    }

    return timer;
}

void canSendCruiseControl() {
    const uint32_t ID = 0x193;

    static uint8_t last_kmh = 0xFE;
    static bool last_enabled = false;

    uint8_t kmh = s_input.cruise_enabled ? min(s_input.cruise_speed, (uint16_t)250) : 0xFE;
    uint8_t cruise_status = s_input.cruise_enabled ? 0xF4 : 0xF1; // 0xF5 for mph!
    uint8_t cc_flag = s_input.cruise_enabled ? 0x58 : 0x50;
    uint8_t speed_update = (kmh != last_kmh || s_input.cruise_enabled != last_enabled) ? 0x01 : 0x00;

    last_kmh = kmh;
    last_enabled = s_input.cruise_enabled;

    uint8_t frame[8] = {
        getCruiseTimer(100),
        kmh,
        cruise_status,
        0x00,
        0xF8,
        cc_flag,
        speed_update,
        0x00
    };

    sendCAN(ID, frame);
}

// CAN read
#define FRAME_SIZE 14
static uint8_t buffer[FRAME_SIZE];

typedef void (*FrameHandler)(const uint8_t* frame);

typedef struct {
    uint32_t id;
    FrameHandler handler;
} CanHandlerEntry;

void handle1B4(const uint8_t* data) {
    int16_t rawHigh = (int16_t)data[1] - 192;
    uint16_t rawLow = data[0];
    uint16_t rawSpeed = ((uint16_t)rawHigh * 256 + rawLow);
    uint16_t mph = rawSpeed / 16;

    // Integer-based km/h conversion with rounding
    uint16_t kmh = (uint32_t)(mph * 160934 + 50000) / 100000;

    const char* handbrake = (data[5] & 0x02) ? "ON" : "OFF";

    pc.printf("[CAN1B4] Speed: %u km/h, Handbrake: %s\n", kmh, handbrake);
}

void handle330(const uint8_t* data) {
    uint8_t avgFuel = data[3];
    uint8_t left = data[4];
    uint8_t right = data[5];
    uint16_t rawRange = (data[7] << 8) | data[6];
    uint16_t range = rawRange / 16;

    pc.printf("[CAN330] AvgFuel: %u L, L: %u, R: %u, Range: %u km\n",
        avgFuel, left, right, range);
}

void handle2C0(const uint8_t* data) {
    pc.printf("[CAN2C0] Brightness %X %X %X %X\n",
        data[0], data[1], data[2], data[3]);
}

static const CanHandlerEntry handler_table[] = {
#ifdef READ_FRAMES_FROM_CLUSTER_1B4
    { 0x000001B4, handle1B4 },
#endif
#ifdef READ_FRAMES_FROM_CLUSTER_330
    { 0x00000330, handle330 },
#endif
#ifdef READ_FRAMES_FROM_CLUSTER_2C0
    { 0x000002C0, handle2C0 },
#endif
};

static const size_t handler_count = sizeof(handler_table) / sizeof(handler_table[0]);

static bool match_id(const uint8_t* buf, uint32_t id) {
    return
        buf[0] == ((id >> 24) & 0xFF) &&
        buf[1] == ((id >> 16) & 0xFF) &&
        buf[2] == ((id >> 8) & 0xFF) &&
        buf[3] == (id & 0xFF);
}

void readCANSerial(void) {
    while (canSerial.available()) {
        for (int i = 0; i < FRAME_SIZE - 1; ++i) {
            buffer[i] = buffer[i + 1];
        }
        buffer[FRAME_SIZE - 1] = canSerial.read();

        // check all handlers
        for (size_t i = 0; i < handler_count; ++i) {
            if (match_id(buffer, handler_table[i].id)) {
                handler_table[i].handler(buffer + 4);
                break;
            }
        }
    }
}

// CAN write queue
const int MaxQueueSize = 64;
void (*canQueue[MaxQueueSize])();
int queueHead = 0;
int queueTail = 0;

bool queueIsEmpty() { return queueHead == queueTail; }
bool queueIsFull() { return ((queueTail + 1) % MaxQueueSize) == queueHead; }
void queuePush(void (*f)()) { if (!queueIsFull()) { canQueue[queueTail] = f; queueTail = (queueTail + 1) % MaxQueueSize; } }
void (*queuePop())() { if (!queueIsEmpty()) { void (*f)() = canQueue[queueHead]; queueHead = (queueHead + 1) % MaxQueueSize; return f; } return nullptr; }

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(REFUELING_LED_PIN, OUTPUT);

    pc.begin(921600);
    canSerial.begin(115200);

#if defined(__AVR_AT90USB1286__)
    // Pick the slightly slower baudrate as the higher more accurate one does not work
    // with the CAN bus adapter
    UCSR1A &= ~(1 << U2X1);
    UBRR1 = 8;
#endif
}

void loop() {
    uint32_t now_us = micros();
    uint32_t now_ms = now_us / 1000;

    // Main loop is executed every 10 ms
    if (now_ms - lastTime >= 10) {
        lastTime = now_ms;

        // Send every 100 ms
        if (canCounter % 10 == 0) {
            queuePush(canSendIgnitionFrame);
            queuePush(canSendEngineTempAndFuelInjection);
            queuePush(canSendGearboxData);
            queuePush(canSendSteeringWheel);
            queuePush(canSendDmeStatus);
            queuePush(canSendCruiseControl);
            queuePush(canSendVehicleDynamics);
        }
        // Send every 50 ms
        if (canCounter % 5 == 1) {
            queuePush(canSendRPM);
            queuePush(canSendSpeed);
            queuePush(canSendTcSymbol);
            queuePush(canSendEscSymbol);
        }
        // Send every 200 ms (group 1)
        if (canCounter % 20 == 7) {
            queuePush(canSendLights);
            queuePush(canSendIndicator);
            queuePush(canSendAbs);
            queuePush(canSendAbsCounter);
            queuePush(canSendAirbagCounter);
            queuePush(canSendFuel);
            queuePush(canSendHandbrake);
        }
        // Send every 200 ms (symbols)
        if (canCounter % 20 == 13) {
            queuePush(canSendEngineTempYellowSymbol);
            queuePush(canSendEngineTempRedSymbol);
            queuePush(canSendCheckEngineSymbol);
            queuePush(canSendClutchTempSymbol);
            queuePush(canSendOilWarningSymbol);
            queuePush(canSendBatteryWarningSymbol);
            queuePush(canSendCustomSymbol);
            queuePush(canSendBrakeTempSymbol);
            queuePush(canSendTireDeflatedFl);
            queuePush(canSendTireDeflatedFr);
            queuePush(canSendTireDeflatedRl);
            queuePush(canSendTireDeflatedRr);
            queuePush(canSendTireDeflatedAll);
            queuePush(canSendRadiatorSymbol);
            queuePush(canSendDoorOpenLeft);
            queuePush(canSendDoorOpenRight);
            queuePush(canSendTailgateOpen);
            queuePush(canSendEscDisabledSymbol);
            queuePush(canSendBeaconSymbol);
        }
        // Send every 500 ms
        if (canCounter % 50 == 5) {
            queuePush(canSuppressSos);
            queuePush(canSuppressService);
        }
        // Send every 1 s
        if (canCounter % 100 == 35) {
            queuePush(canSendTime);
        }
        // Send every 10 s
        if (canCounter % 1000 == 47) {
            queuePush(canSendOilLevel);
        }

        canCounter++;
    }

    // Allow 3 ms time for the serial CAN bus to transmit the frame. With 115200 baud
    // rate to Serial CAN bus and 100 kbs CAN bus this should be enough but 1-2 ms isn't
    if (now_us - lastTaskTime >= 3000) {
        void (*task)() = queuePop();
        if (task) {
            lastTaskTime = now_us;
            task();
        }
    }

    readSerial();
    parseTelemetryLine();

#ifdef READ_FRAMES_FROM_CLUSTER
    readCANSerial();
#endif
}
