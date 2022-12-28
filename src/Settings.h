#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

#include "config.h"

class Settings {
public:
    struct Time {
        uint8_t hour;
        uint8_t minute;
    };

    // Settings
    uint8_t palette;
    uint8_t brightness;

    bool ntpEnabled;
    String ntpServer;
    uint32_t syncInterval;

    int timezone;

    // night light
#ifdef NIGHTMODE
    bool nmEnable;
    Time nmStartTime;
    Time nmEndTime;
#endif


public:
    Settings() { }

    bool loadSettings();
    void saveSettings();

private:
    static constexpr const char *cfgFile = "/config/wordclock.json";
};

extern Settings settings;
