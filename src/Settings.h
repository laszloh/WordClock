#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <FastLED.h>

#include "c++23.h"
#include "config.h"

namespace data {

extern const TProgmemRGBPalette16 RedBlue_p FL_PROGMEM;
extern const TProgmemRGBPalette16 RedFire_p FL_PROGMEM;
extern const TProgmemRGBPalette16 BlueIce_p FL_PROGMEM;
extern const TProgmemRGBPalette16 Green_p FL_PROGMEM;
extern const TProgmemRGBPalette16 Red_p FL_PROGMEM;

const std::array colorPalettes
    = std::make_array(&RedBlue_p, &RedFire_p, &BlueIce_p, &RainbowColors_p, &PartyColors_p, &OceanColors_p, &ForestColors_p, &Green_p);

const std::array paletteNames = std::make_array("Red-Blue", "Red-Fire", "Blue-Ice", "Rainbow", "Party", "Ocean", "Forest", "Green");

} // namespace config

struct ColorPalette {
    size_t currentPalette;

    ColorPalette &operator++(int) {
        currentPalette++;
        if(currentPalette >= data::colorPalettes.size())
            currentPalette = 0;
        return *this;
    }

    operator size_t() const { return currentPalette; }

    ColorPalette &operator=(const ColorPalette &rhs) {
        currentPalette = rhs.currentPalette;
        return *this;
    }

    ColorPalette &operator=(const size_t &rhs) {
        currentPalette = rhs;
        return *this;
    }
};

struct TimeStruct {
    int hour;
    int minute;

    bool operator<(const struct tm &tm) const {
        if(hour < tm.tm_hour) {
            // hours bigger, so do not need to check further
            return true;
        } else if(hour == tm.tm_hour && minute < tm.tm_min) {
            // hour is same, but minutes bigger
            return true;
        }
        return false;
    }

    bool operator>(const struct tm &tm) const { return !operator<(tm); }

    String toString() const {
        char buf[16];
        snprintf(buf, sizeof(buf), "%02d:%02d", hour, minute);
        return String(buf);
    }

    bool parseString(const char *str) {
        int ret = sscanf(str, "%d:%d", &hour, &minute);
        return ret == 2;
    }
};

enum class Brightness : uint8_t {
    low = 0,
    mid,
    high,
#ifdef NIGHTMODE
    night,
#endif
    END_OF_LIST
};
Brightness &operator++(Brightness &b, int);
inline uint8_t BrightnessToIndex(Brightness &b) {
    if(b >= Brightness::END_OF_LIST)
        b = Brightness::mid;
    return std::to_underlying(b);
}


class Settings {
public:
    // Settings
    ColorPalette palette;
    Brightness brightness;

    bool wifiEnable;

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

    void loop();

    bool loadSettings();
    void saveSettings();

    void cycleBrightness();
    void cyclePalette();
    void requestAsyncSave() { saveRequest = true; }

    static void resetSettings();

private:
    static constexpr const char *cfgFile = "/config/wordclock.json";
    bool saveRequest{false};
};

extern Settings settings;
