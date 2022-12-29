#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

#include "c++23.h"
#include "config.h"
 
struct TimeStruct {
    uint8_t hour;
    uint8_t minute;
};
bool operator<(const TimeStruct &t, const struct tm &tm);
bool operator>(const TimeStruct &t, const struct tm &tm);

enum class Brightness : uint8_t { night = 0, low, mid, high, END_OF_LIST };
Brightness &operator++(Brightness &b, int);

class Settings {
public:
    // Settings
    uint8_t palette;
    Brightness brightness;

    bool ntpEnabled;
    String ntpServer;
    uint32_t syncInterval;

    int timezone;

    // night light
#ifdef NIGHTMODE
    bool nmEnable;
    TimeStruct nmStartTime;
    TimeStruct nmEndTime;
#endif


public:
    Settings() { }

    bool loadSettings();
    void saveSettings();

private:
    static constexpr const char *cfgFile = "/config/wordclock.json";
};

extern Settings settings;
