#include "esp-hal-log.h"
#include <Arduino.h>

const char *pathToFileName(const char *path) {
    size_t i = 0;
    size_t pos = 0;
    char *p = (char *)path;
    while(*p) {
        i++;
        if(*p == '/' || *p == '\\') {
            pos = i;
        }
        p++;
    }
    return path + pos;
}

int log_printf(PGM_P fmt, ...) {
    va_list arg;
    char temp[64];
    char *buffer = temp;

    va_start(arg, fmt);
    size_t len = vsnprintf_P(temp, sizeof(temp), fmt, arg);
    va_end(arg);
    if(len > sizeof(temp) - 1) {
        buffer = new char[len + 1];
        if(!buffer)
            return 0;
        va_start(arg, fmt);
        vsnprintf_P(buffer, len + 1, fmt, arg);
        va_end(arg);
    }
    len = Serial.write((const uint8_t *)buffer, len);
    if(buffer != temp)
        delete[] buffer;
    return len;
}
