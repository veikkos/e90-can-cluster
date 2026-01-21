#pragma once

#include <Arduino.h>
#include <stdarg.h>

template<typename T>
void serial_printf(T& serial, const char* format, ...) {
    va_list args;
    va_start(args, format);

    char buffer[128];
    vsnprintf(buffer, sizeof(buffer), format, args);
    serial.print(buffer);
    
    va_end(args);
}
