#include "mbed.h"
#include <ctime>
#include <cstring>
#include <math.h>

DigitalOut led1(LED1);
DigitalOut led2(LED2);

// Serial setup
BufferedSerial canSerial(p9, p10, 115200);
BufferedSerial pc(USBTX, USBRX, 115200);

// CAN state
Timer canTimer;
uint32_t lastTime = 0;
uint16_t canCounter = 0;

// App state
bool s_ignition = true;
bool s_indicator_left = false;
bool s_indicator_right = true;
uint16_t s_time_year = 2024;
uint16_t s_rpm = 2000;
uint16_t s_speed = 50;
bool bc_pressed = false;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/*
  STATE ENUMS
*/
typedef enum{
  I_OFF = 0,
  I_LEFT = 1,
  I_RIGHT = 2,
  I_HAZZARD = 3,
} INDICATOR;


/*
  CAN ENUMS
*/
typedef enum{
  L_BRAKE = 0b10000000,
  L_FOG = 0b01000000,
  L_BACKLIGHT = 0b00000100,
  L_MAIN = 0b00000010,
  L_DIP = 0b00100000,
} CAN_LIGHTS;

// --- CAN Send Helpers ---
void sendCAN(uint32_t id, const uint8_t* data) {
    uint8_t buf[14] = {
        (uint8_t)((id >> 24) & 0xFF), (uint8_t)((id >> 16) & 0xFF),
        (uint8_t)((id >> 8) & 0xFF), (uint8_t)(id & 0xFF),
        0x00, 0x00
    };
    memcpy(&buf[6], data, 8);
    canSerial.write(buf, 14);
}

// --- IGNITION ---
void canSendIgnitionFrame() {
    static uint8_t counter_on = 0xE2;
    const uint32_t ID = 0x130;
    uint8_t data[8] = {0x45, 0x42, 0x69, 0x8F, counter_on++, 0, 0, 0};
    sendCAN(ID, data);
}

void canSendIgnitionState() {
    const uint32_t CAN_ID = 0x26E;
    static uint8_t frame[8] = {0};

    switch (2) { // You define this enum or constant elsewhere
        case 0:  // OFF
            frame[0] = 0x00; frame[1] = 0x00; frame[2] = 0x3F;
            break;
        case 1:  // Position 1
            frame[0] = 0x00; frame[1] = 0x40; frame[2] = 0x7F;
            break;
        case 2:  // Position 2
            frame[0] = 0x40; frame[1] = 0x40; frame[2] = 0x7F;
            break;
    }

    frame[3] = 0x50;
    for (int i = 4; i < 8; ++i) frame[i] = 0xFF;

    sendCAN(CAN_ID, frame);
}

// --- RPM ---
void canSendRPM() {
    const uint32_t ID = 0x00AA;
    uint16_t rpm_val = s_rpm * 4;
    uint8_t data[8] = {0x5F, 0x59, 0xFF, 0x00,
                       (uint8_t)(rpm_val & 0xFF), (uint8_t)(rpm_val >> 8),
                       0x80, 0x99};
    sendCAN(ID, data);
}

// --- SPEED ---
void canSendSpeed() {
    const uint16_t CAN_ID = 0x1A6;
    static uint16_t last_speed_counter = 0;
    static uint16_t last_tick_counter = 0;
    uint32_t delta_ms = 100;
    uint16_t speed_mph = (s_speed * 621) / 1000;
    uint16_t current_speed_counter = speed_mph + last_speed_counter;

    // Tick counter (still 100 per 50ms) → now scale accordingly:
    // delta_tick_counter = (delta_ms * 100) / 50 = delta_ms * 2
    uint16_t delta_tick_counter = delta_ms * 2;
    uint16_t tick_counter = last_tick_counter + delta_tick_counter;

    // Build CAN frame
    uint8_t low = current_speed_counter & 0xFF;
    uint8_t high = current_speed_counter >> 8;

    uint8_t frame[8] = {
        low, high,
        low, high,
        low, high,
        (uint8_t)(tick_counter & 0xFF),
        (uint8_t)(0xF0 | ((tick_counter >> 8) & 0x0F))
    };

    sendCAN(CAN_ID, frame);

    // Save state
    last_speed_counter = current_speed_counter;
    last_tick_counter = tick_counter;
}

// --- STEERING WHEEL (stub) ---
void canSendSteeringWheel() {
    const uint32_t ID = 0x0C4;
    static uint8_t frame[8] = {0x83, 0xFD, 0xFC, 0x00, 0x00, 0xFF, 0xF1, 0x00};

    frame[1] = 0;  // reset as in original logic
    frame[2] = 0;

    sendCAN(ID, frame);
}

// --- LIGHTS (stub) ---
void canSendLights() {
    const uint32_t ID = 0x21A;
    static uint8_t frame[8] = {0x00, 0x00, 0xF7, 0, 0, 0, 0, 0};

    uint16_t lights = 0;
    if (true) lights |= L_BACKLIGHT;
    if (false)     lights |= L_DIP;
    if (true)    lights |= L_MAIN;
    if (false)     lights |= L_FOG;

    frame[0] = lights & 0xFF;

    sendCAN(ID, frame);
}

void canSendIndicator() {
    const uint32_t ID = 0x1F6;
    uint8_t indicator_frame[8] = {0x80, 0xF0, 0, 0, 0, 0, 0, 0};  // default: off

    sendCAN(ID, indicator_frame);
}

// --- ABS (stub) ---
void canSendAbs() {
    const uint32_t ID = 0x19E;
    static uint8_t frame[8] = {0x00, 0xE0, 0xB3, 0xFC, 0xF0, 0x43, 0x00, 0x65};

    frame[2] = ((((frame[2] >> 4) + 3) << 4) & 0xF0) | 0x03;

    sendCAN(ID, frame);
}

// --- ENGINE TEMP (stub) ---
void canSendEngineTemp() {
    const uint32_t ID = 0x1D0;
    static uint8_t frame[8] = {0x8B, 0xFF, 0x63, 0xCD, 0x5D, 0x37, 0xCD, 0xA8};

    frame[0] = 30 + 48;
    frame[2]++;

    sendCAN(ID, frame);
}

// --- ABS Counter (stub) ---
void canSendAbsCounter() {
    const uint32_t ID = 0x0C0;
    static uint8_t frame[8] = {0xF0, 0xFF, 0, 0, 0, 0, 0, 0};

    sendCAN(ID, frame);
    frame[0] = ((frame[0] + 1) | 0xF0);
}


// --- Airbag Counter (stub) ---
void canSendAirbagCounter() {
    const uint32_t ID = 0x0D7;
    static uint8_t frame[8] = {0xC3, 0xFF, 0, 0, 0, 0, 0, 0};

    sendCAN(ID, frame);
    frame[0]++;
}


// --- Fuel (stub) ---
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

// --- Handbrake (stub) ---
void canSendHandbrake() {
    const uint32_t ID = 0x34F;
    static uint8_t frame[8] = {0xFE, 0xFF, 0, 0, 0, 0, 0, 0};

    frame[0] = false ? 0xFE : 0xFD;

    sendCAN(ID, frame);
}

// --- Time ---
void canSendTime() {
    const uint32_t ID = 0x39E;
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    uint16_t year = t->tm_year + 1900;
    uint8_t data[8] = {
        (uint8_t)t->tm_hour, (uint8_t)t->tm_min, (uint8_t)t->tm_sec,
        (uint8_t)t->tm_mday, 4, // (uint8_t)((t->tm_mon + 1) << 4) | 0x0F,
        (uint8_t)(year & 0xFF), (uint8_t)((year >> 8) & 0xFF), 0xF2
    };
    sendCAN(ID, data);
}

void canSendBCButtonPress() {
    const uint32_t ID = 0x0C4;
    uint8_t frame[8] = {0x83, 0x01, 0x00, 0x00, 0x00, 0xFF, 0xF1, 0x00}; // 0x01 = BC button press
    sendCAN(ID, frame);
}

void canSendBCButtonRelease() {
    const uint32_t ID = 0x0C4;
    uint8_t frame[8] = {0x83, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xF1, 0x00}; // 0x00 = no button pressed
    sendCAN(ID, frame);
}

void canSendGearSelector() {
    const uint32_t ID = 0x1A0;
    static uint8_t frame[8] = {0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Start in 'P'

    static int gear_step = 2;
    switch (gear_step) {
        case 0: frame[0] = 0x50; break; // P
        case 1: frame[0] = 0x30; break; // N
        case 2: frame[0] = 0x20; break; // D
        case 3: frame[0] = 0x10; break; // R
    }

    sendCAN(ID, frame);
}

// --- CONFIG SEND COMMAND ---
void sendCommand(const char* cmd) {
    pc.write(">> ", 3); pc.write(cmd, strlen(cmd));
    canSerial.write(cmd, strlen(cmd));
    ThisThread::sleep_for(100ms);

    char buf[64] = {0}; size_t index = 0;
    while (canSerial.readable() && index < sizeof(buf) - 1) {
        char c;
        if (canSerial.read(&c, 1)) buf[index++] = c;
    }

    buf[index] = '\0';
    pc.write("<< ", 3); pc.write(buf, strlen(buf)); pc.write("\r\n", 2);
}

// --- MAIN LOOP ---
const int MaxQueueSize = 32;
typedef void (*CanFunction)();

CanFunction canQueue[MaxQueueSize];
int queueHead = 0; // Points to next item to pop
int queueTail = 0; // Points to next slot to push

inline bool queueIsEmpty() {
    return queueHead == queueTail;
}

inline bool queueIsFull() {
    return ((queueTail + 1) % MaxQueueSize) == queueHead;
}

inline void queuePush(CanFunction f) {
    if (!queueIsFull()) {
        canQueue[queueTail] = f;
        queueTail = (queueTail + 1) % MaxQueueSize;
    }
    // else: drop the message silently (or add logging if needed)
}

inline CanFunction queuePop() {
    if (!queueIsEmpty()) {
        CanFunction f = canQueue[queueHead];
        queueHead = (queueHead + 1) % MaxQueueSize;
        return f;
    }
    return nullptr;
}

int main() {
    canTimer.start();

    while (true) {
        uint32_t now = canTimer.elapsed_time().count() / 1000;

        if (now - lastTime >= 10) {
            lastTime = now;
            canCounter++;

            // Schedule messages by pushing to queue

            // Every 100 ms
            if (canCounter % 10 == 0) {
                queuePush(canSendIgnitionFrame);
                queuePush(canSendIgnitionState);
                queuePush(canSendSpeed);
            }

            // Every 50 ms
            if (canCounter % 5 == 0) {
                queuePush(canSendRPM);
                if (!bc_pressed) {
                    queuePush(canSendSteeringWheel);
                }
            }

            // Every 200 ms
            if (canCounter % 20 == 0) {
                queuePush(canSendLights);
                queuePush(canSendIndicator);
                queuePush(canSendAbs);
                queuePush(canSendEngineTemp);
                queuePush(canSendAbsCounter);
                queuePush(canSendAirbagCounter);
                queuePush(canSendFuel);
            }

            // Every 500 ms
            if (canCounter % 50 == 0) {
                queuePush(canSendHandbrake);

                if (bc_pressed) {
                    bc_pressed = false;
                    queuePush(canSendBCButtonRelease);
                    led1 = false;
                }
            }

            // Every 1 second
            if (canCounter % 100 == 0) {
                queuePush(canSendTime);
                led2 = !led2;

                s_rpm += 500;
                if (s_rpm > 6000)
                    s_rpm = 1000;
            }

            // Every 4 seconds
            if (canCounter % 400 == 0) {
                queuePush(canSendBCButtonPress);
                queuePush(canSendGearSelector);
                
                bc_pressed = true;
                led1 = true;
            }
        }

        // Send one message per ms if available
        CanFunction task = queuePop();
        if (task) {
            task();
        }

        ThisThread::sleep_for(2ms);
    }
}
