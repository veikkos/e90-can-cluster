#pragma once

#include <stdint.h>
#include <stddef.h>

void canBegin();
void canSend(uint32_t id, const uint8_t* data);

typedef void (*CanFrameHandler)(const uint8_t* data);

struct CanHandlerEntry {
    uint32_t id;
    CanFrameHandler handler;
};

void canPoll(const CanHandlerEntry* handlers, size_t count);
