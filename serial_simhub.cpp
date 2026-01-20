#include <Arduino.h>
#include "types.h"
#include "serial.h"
#include "cluster.h"

extern SInput s_input;

#define ASCII_BUF_SIZE 128

static char buf[ASCII_BUF_SIZE];
static uint8_t pos = 0;
static bool line_ready = false;

void simHubSerialRead() {
    while (pc.available()) {
        char c = pc.read();
        if (c == '\n') {
            buf[pos] = '\0';
            line_ready = true;
            pos = 0;
        } else if (pos < ASCII_BUF_SIZE - 1) {
            buf[pos++] = c;
        }
    }
}

static void parseToken(char* t) {
    if (strncmp(t, "RPM=", 4) == 0) {
        s_input.rpm = atoi(t + 4);
    } else if (strncmp(t, "SPD=", 4) == 0) {
        s_input.speed = atoi(t + 4);
    } else if (strncmp(t, "GEAR=", 5) == 0) {
        int g = atoi(t + 5);
        if (g <= 0) {
            s_input.currentGear = NEUTRAL;
            s_input.manualGear = NONE;
        } else {
            s_input.currentGear = DRIVE;
            s_input.manualGear = (GEAR_MANUAL)min(g, NUMBER_OF_GEARS);
        }
    } else if (strncmp(t, "FUEL=", 5) == 0) {
        s_input.fuel = constrain(atoi(t + 5), 0, 1000);
    } else if (strncmp(t, "OIL=", 4) == 0) {
        s_input.oil_temp = atoi(t + 4);
    }
}

void simHubSerialParse() {
    if (!line_ready) return;
    line_ready = false;

    if (buf[0] != 'S') return;

    char* p = buf + 1;
    char* tok;
    while ((tok = strtok(p, ",")) != nullptr) {
        parseToken(tok);
        p = nullptr;
    }

    s_input.ignition = IG_ON;
    s_input.engine_running = s_input.rpm > 0;
    s_input.light_lowbeam = true;
}
