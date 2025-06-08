#include "mbed.h"
#include <ctime>
#include <cstring>
#include <math.h>

DigitalOut led1(LED1);
DigitalOut led2(LED2);

BufferedSerial canSerial(p9, p10, 115200);
BufferedSerial pc(USBTX, USBRX, 115200);

Timer canTimer;
uint32_t lastTime = 0;
uint16_t canCounter = 0;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef enum {
    I_OFF = 0,
    I_LEFT = 1,
    I_RIGHT = 2,
    I_HAZZARD = 3,
} INDICATOR;

typedef enum {
    L_BRAKE = 0b10000000,
    L_FOG = 0b01000000,
    L_BACKLIGHT = 0b00000100,
    L_MAIN = 0b00000010,
    L_DIP = 0b00100000,
} CAN_LIGHTS;

struct SInput {
    bool ignition = true;
    INDICATOR indicator_state = I_OFF;
    uint16_t time_year = 2024;
    uint16_t rpm = 2000;
    uint16_t speed = 20;
    uint8_t water_temp = 90;
    bool light_backlight = true;
    bool light_main = true;
    bool light_dip = false;
    bool light_fog = false;
    bool handbrake = false;
};

SInput s_input;

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
    static uint8_t counter_on = 0xE2;
    const uint32_t ID = 0x130;
    uint8_t data[8] = {0x45, 0x42, 0x69, 0x8F, counter_on++, 0, 0, 0};
    sendCAN(ID, data);
}

void canSendIgnitionState() {
    const uint32_t ID = 0x26E;
    static uint8_t frame[8] = {0};

    switch (2) {
        case 0: frame[0] = 0x00; frame[1] = 0x00; frame[2] = 0x3F; break;
        case 1: frame[0] = 0x00; frame[1] = 0x40; frame[2] = 0x7F; break;
        case 2: frame[0] = 0x40; frame[1] = 0x40; frame[2] = 0x7F; break;
    }

    frame[3] = 0x50;
    for (int i = 4; i < 8; ++i) frame[i] = 0xFF;
    sendCAN(ID, frame);
}

void canSendRPM() {
    const uint32_t ID = 0x00AA;
    uint16_t rpm_val = s_input.rpm * 4;
    uint8_t data[8] = {0x5F, 0x59, 0xFF, 0x00,
                       (uint8_t)(rpm_val & 0xFF), (uint8_t)(rpm_val >> 8),
                       0x80, 0x99};
    sendCAN(ID, data);
}

void canSendSpeed() {
    const uint32_t ID = 0x1A6;
    static uint16_t last_speed_counter = 0;
    static uint16_t last_tick_counter = 0;
    uint32_t delta_ms = 100;
    uint16_t speed_mph = (s_input.speed * 621) / 1000;
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

    uint16_t lights = 0;
    if (s_input.light_backlight) lights |= L_BACKLIGHT;
    if (s_input.light_dip)       lights |= L_DIP;
    if (s_input.light_main)      lights |= L_MAIN;
    if (s_input.light_fog)       lights |= L_FOG;

    frame[0] = lights & 0xFF;
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
    static uint8_t brake_frame_values[] = {0x43, 0x65, 0x67};

    uint8_t frame[8] = {
        0x00, 0xE0, 0x00, 0xFC, 0xCC, 0x00, 0x00, 0x00
    };

    // Byte 2 rotation (high nibble cycles, low nibble is always 0x03)
    uint8_t rotate_nibble = (counter % 3) << 4;
    frame[2] = rotate_nibble | 0x03;

    // Braking status
    if (true) {
        frame[5] = brake_frame_values[counter % 3];
        frame[6] = 12 + (counter % 20);  // Fake pressure
    } else {
        frame[5] = 0x00;
        frame[6] = 0x00;
    }

    frame[7] = counter;  // Varying counter
    counter++;

    sendCAN(ID, frame);
}

void canSendEngineTemp() {
    const uint32_t ID = 0x1D0;
    static uint8_t frame[8] = {0x8B, 0xFF, 0x63, 0xCD, 0x5D, 0x37, 0xCD, 0xA8};
    frame[0] = s_input.water_temp + 48;
    frame[2]++;
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

void canSendFuel() {
    const uint32_t ID = 0x349;
    static uint8_t frame[8] = {0};
    uint16_t level = min(100 + (500 * 8), 8000);
    frame[0] = level & 0xFF;
    frame[1] = (level >> 8);
    frame[2] = frame[0];
    frame[3] = frame[1];
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
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    uint16_t year = t->tm_year + 1900;

    uint8_t data[8] = {
        (uint8_t)t->tm_hour, (uint8_t)t->tm_min, (uint8_t)t->tm_sec,
        (uint8_t)t->tm_mday, (uint8_t)(t->tm_mon + 1),
        (uint8_t)(year & 0xFF), (uint8_t)((year >> 8) & 0xFF), 0xF2
    };
    sendCAN(ID, data);
}

void canSendGearSelector() {
    const uint32_t ID = 0x1A0;
    static uint8_t frame[8] = {0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    static int gear_step = 2;
    switch (gear_step) {
        case 0: frame[0] = 0x50; break;
        case 1: frame[0] = 0x30; break;
        case 2: frame[0] = 0x20; break;
        case 3: frame[0] = 0x10; break;
    }
    sendCAN(ID, frame);
}

void canSendDmeStatus() {
    const uint32_t ID = 0x12F;
    uint8_t frame[8] = {0x3F, 0x00, 0x00, 0x00, 0x00, 0x40, 0x01, 0x00};
    sendCAN(ID, frame);
}

void canSendOutsideTemp() {
    const uint32_t ID = 0x366;
    uint8_t frame[] = { 0x78, 0x50, 0x14, 0xFC };
    sendCAN(ID, frame);
}

const uint32_t ID_ERROR_LIGHT = 0x592;

enum ErrorLightID : uint16_t {
    CHECK_ENGINE = 34,
    OVERHEAT = 39,
    PARTICLE = 49,
    DTC = 184,
    DTC_DEACTIVATED = 36,
    GEAR_ISSUE = 348,
    OIL_RED = 212,
    BATTERY_RED = 213,
    SOS_ERROR = 299,
    FUEL_WARNING = 275,
    SERVICE_LIGHT = 281
};

void canSendErrorLight(ErrorLightID light_id, bool enable) {
    const uint8_t ON = 0x31;
    const uint8_t OFF = 0x30;

    uint8_t frame[] = {
        0x40,
        static_cast<uint8_t>(light_id & 0xFF),
        static_cast<uint8_t>((light_id >> 8) & 0xFF),
        enable ? ON : OFF,
        0xFF, 0xFF, 0xFF, 0xFF
    };
    sendCAN(ID_ERROR_LIGHT, frame);
}

void canSuppressService() {
    canSendErrorLight(SERVICE_LIGHT, false);
}

void canSuppressSos() {
    canSendErrorLight(SOS_ERROR, false);
}

const int MaxQueueSize = 32;
typedef void (*CanFunction)();

CanFunction canQueue[MaxQueueSize];
int queueHead = 0;
int queueTail = 0;

inline bool queueIsEmpty() { return queueHead == queueTail; }
inline bool queueIsFull() { return ((queueTail + 1) % MaxQueueSize) == queueHead; }
inline void queuePush(CanFunction f) { if (!queueIsFull()) { canQueue[queueTail] = f; queueTail = (queueTail + 1) % MaxQueueSize; } }
inline CanFunction queuePop() { if (!queueIsEmpty()) { CanFunction f = canQueue[queueHead]; queueHead = (queueHead + 1) % MaxQueueSize; return f; } return nullptr; }

int main() {
    canTimer.start();

    while (true) {
        uint32_t now = canTimer.elapsed_time().count() / 1000;

        // Main loop is executed every 10 ms
        if (now - lastTime >= 10) {
            lastTime = now;
            canCounter++;

            // Send every 100 ms
            if (canCounter % 10 == 0) {
                queuePush(canSendIgnitionFrame);
                queuePush(canSendIgnitionState);
                queuePush(canSendSpeed);
            }
            // Send every 50 ms
            if (canCounter % 5 == 1) {
                queuePush(canSendRPM);
                queuePush(canSendSteeringWheel);
            }
            // Send every 200 ms
            if (canCounter % 20 == 7) {
                queuePush(canSendLights);
                queuePush(canSendIndicator);
                queuePush(canSendAbs);
                queuePush(canSendEngineTemp);
                queuePush(canSendAbsCounter);
                queuePush(canSendAirbagCounter);
                queuePush(canSendFuel);
                queuePush(canSendGearSelector);
            }
            // Send every 500 ms
            if (canCounter % 50 == 5) {
                queuePush(canSendHandbrake);
                queuePush(canSuppressSos);
                queuePush(canSuppressService);
            }
            // Send every 1 s
            if (canCounter % 100 == 35) {
                queuePush(canSendTime);
                queuePush(canSendDmeStatus);
                queuePush(canSendOutsideTemp);
                led2 = !led2;
                s_input.rpm += 500;
                if (s_input.rpm > 6000)
                    s_input.rpm = 1000;
            }
        }

        CanFunction task = queuePop();
        if (task) {
            task();
        }

        // Allow 2 ms time for the serial CAN bus to transmit the frame. With 115200 baud
        // rate to Serial CAN bus and 100 kbs CAN bus this should be enough but 1 ms isn't
        ThisThread::sleep_for(2ms);
    }
}
