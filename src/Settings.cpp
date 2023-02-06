#include <Arduino.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <LittleFS.h>

#include "Settings.h"
#include "WordClock.h"
#include "esp-hal-log.h"
#include "genTimezone.h"

Settings settings;


namespace data {

const TProgmemRGBPalette16 RedBlue_p FL_PROGMEM = {0x2800E0, 0x5000B4, 0x790089, 0xA2005E, 0xCB0033, 0xF40008, 0xE7001A, 0xDA002C,
                                                   0xCD003F, 0xC00051, 0xB40064, 0xA30079, 0x92008F, 0x8100A4, 0x7000BA, 0x6000CF};
const TProgmemRGBPalette16 RedFire_p FL_PROGMEM = {0x0010E0, 0x0019D9, 0x0023D3, 0x002CCC, 0x0036C6, 0x0040C0, 0x004FC0, 0x005EC0,
                                                   0x006DC0, 0x007CC0, 0x008CC0, 0x0078C9, 0x0064D3, 0x0050DC, 0x003CE6, 0x0028EF};
const TProgmemRGBPalette16 BlueIce_p FL_PROGMEM = {0xD4FF7F, 0xD3F872, 0xD2F265, 0xD1EC59, 0xD0E64C, 0xD0E040, 0xD9B333, 0xE28626,
                                                   0xEC5919, 0xF52C0C, 0xFF0000, 0xFF2600, 0xFF4C00, 0xFF7200, 0xFF9800, 0xFFBE00};
const TProgmemRGBPalette16 Green_p FL_PROGMEM = {0x00FC7C, 0x00FC7C, 0x00FC7C, 0x00FC7C, 0x00FC7C, 0x00FC7C, 0x00FC7C, 0x00FC7C,
                                                 0x00FC7C, 0x00FC7C, 0x00FC7C, 0x00FC7C, 0x00FC7C, 0x00FC7C, 0x00FC7C, 0x00FC7C};
const TProgmemRGBPalette16 Red_p FL_PROGMEM = {0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000,
                                               0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000};

} // namespace data


void convertFromJson(JsonVariantConst src, ColorPalette &b) { b.currentPalette = src.as<size_t>(); }
void convertToJson(const ColorPalette &b, JsonVariant dst) { dst.set(b.currentPalette); }
bool canConvertFromJson(JsonVariantConst src, const ColorPalette &) { return src.is<size_t>(); }

void convertFromJson(JsonVariantConst src, TimeStruct &t) {
    t.hour = src["hour"];
    t.minute = src["minute"];
}
void convertToJson(const TimeStruct &t, JsonVariant dst) {
    dst["hour"] = t.hour;
    dst["minute"] = t.minute;
}
bool canConvertFromJson(JsonVariantConst src, const TimeStruct &) { return src["hour"].is<int>() && src["minute"].is<int>(); }

void convertFromJson(JsonVariantConst src, Brightness &b) { b = static_cast<Brightness>(src.as<uint8_t>()); }
void convertToJson(const Brightness &b, JsonVariant dst) { dst.set(std::to_underlying(b)); }
bool canConvertFromJson(JsonVariantConst src, const Brightness &) {
    return src.is<uint8_t>() && src.as<uint8_t>() < std::to_underlying(Brightness::END_OF_LIST);
}

// Special behavior for Brightness++
Brightness &operator++(Brightness &b, int) {
    b = static_cast<Brightness>(std::to_underlying(b) + 1);
    if(b == Brightness::END_OF_LIST) {
        b = Brightness::low;
    }
    return b;
}


void Settings::loop() {
    if(saveRequest) {
        saveRequest = false;
        saveSettings();
    }
}

bool Settings::loadSettings() {
    bool ret = true;
    StaticJsonDocument<1024> doc;

    File f = LittleFS.open(cfgFile, "r");

    if(f) {
        DeserializationError err = deserializeJson(doc, f);
        if(err) {
            log_e("Deserialze Failed: %s (%d)", err.c_str(), err.code());
            ret = false;
        }
    } else {
        log_e("File not found");
        ret = false;
    }
    f.close();

    brightness = doc["brightness"] | Brightness::mid;
    palette = doc["palette"] | 0;

    timezone = doc["timezone"] | TZ_Names::TZ_Europe_Vienna;

    wifiEnable = doc["wifi"] | true;

    ntpEnabled = doc["ntp-enabled"] | true;
    ntpServer = doc["ntp-server"] | "europe.pool.ntp.org";
    syncInterval = doc["ntp-interval"] | 720;

#ifdef NIGHTMODE
    nmEnable = doc["nm-endable"] | true;
    nmAutomatic = doc["nm-automatic"] | true;
    nmStartTime = doc["nm-start"] | TimeStruct{20, 00};
    nmEndTime = doc["nm-end"] | TimeStruct{10, 00};
#endif

    serializeJsonPretty(doc, Serial);

    if(!ret)
        log_w("loaded default value");
    return ret;
}

void Settings::saveSettings() {
    File f = LittleFS.open(cfgFile, "w");
    if(!f) {
        log_e("Failed to open file for writing!");
        f.close();
        return;
    }

    StaticJsonDocument<1024> doc;

    doc["brightness"] = brightness;
    doc["palette"] = palette;

    doc["timezone"] = timezone;

    doc["wifi"] = wifiEnable;

    doc["ntp-enabled"] = ntpEnabled;
    doc["ntp-server"] = ntpServer;
    doc["ntp-interval"] = syncInterval;

#ifdef NIGHTMODE
    doc["nm-endable"] = nmEnable;
    doc["nm-start"] = nmStartTime;
    doc["nm-end"] = nmEndTime;
#endif

    serializeJsonPretty(doc, Serial);
    serializeJson(doc, f);
    f.close();
}

void Settings::resetSettings(){
    LittleFS.remove(cfgFile);
}

void Settings::cyclePalette() {
    settings.palette++;
    saveSettings();

    log_v("Current Palette: %d", settings.palette);
    wordClock.setPalette(true);
}

void Settings::cycleBrightness() {
    settings.brightness++;
    saveSettings();

    log_v("Current Brightness: %d", settings.brightness);
    wordClock.setBrightness(true);
}
