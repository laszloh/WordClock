#include <Arduino.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <LittleFS.h>

#include "Settings.h"
#include "esp-hal-log.h"
#include "genTimezone.h"

Settings settings;

void convertFromJson(JsonVariantConst src, TimeStruct &t) {
    t.hour = src["hour"];
    t.minute = src["minute"];
}
void convertToJson(const TimeStruct &t, JsonVariant dst) {
    dst["hour"] = t.hour;
    dst["minute"] = t.minute;
}
bool canConvertFromJson(JsonVariantConst src, const TimeStruct &) { return src["hour"].is<uint8_t>() && src["minte"].is<uint8_t>(); }

void convertFromJson(JsonVariantConst src, Brightness &b) { b = static_cast<Brightness>(src.as<uint8_t>()); }
void convertToJson(const Brightness &b, JsonVariant dst) { dst.set(std::to_underlying(b)); }
bool canConvertFromJson(JsonVariantConst src, const Brightness &) {
    return src.is<uint8_t>() && src.as<uint8_t>() < std::to_underlying(Brightness::high);
}

bool operator<(const TimeStruct &t, const struct tm &tm) { return t.hour < tm.tm_hour && t.minute < tm.tm_min; }

bool operator>(const TimeStruct &t, const struct tm &tm) { return t.hour > tm.tm_hour && t.minute > tm.tm_min; }

// Special behavior for Brightness++
Brightness &operator++(Brightness &b, int) {
    b = static_cast<Brightness>(std::to_underlying(b) + 1);
    if(b == Brightness::END_OF_LIST)
        b = Brightness::night;
    return b;
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

    brightness = doc["brightness"] | Brightness::mid;
    palette = doc["palette"] | 0;

    timezone = doc["timezone"] | TZ_Names::TZ_Europe_Vienna;

    ntpEnabled = doc["ntp-enabled"] | true;
    ntpServer = doc["ntp-server"] | "europe.pool.ntp.org";
    syncInterval = doc["ntp-interval"] | 720;

#ifdef NIGHTMODE
    nmEnable = doc["nm-endable"] | false;
    nmStartTime = doc["nm-start"] | TimeStruct{20, 00};
    nmEndTime = doc["nm-end"] | TimeStruct{10, 00};
#endif

    if(!ret)
        log_w("loaded default value");
    return ret;
}

void Settings::saveSettings() {
    File f = LittleFS.open(cfgFile, "w");
    StaticJsonDocument<1024> doc;

    doc["brightness"] = brightness;
    doc["palette"] = palette;

    doc["timezone"] = timezone;

    doc["ntp-enabled"] = ntpEnabled;
    doc["ntp-server"] = ntpServer;
    doc["ntp-interval"] = syncInterval;

#ifdef NIGHTMODE
    doc["nm-endable"] = nmEnable;
    doc["nm-start"] = nmStartTime;
    doc["nm-end"] = nmEndTime;
#endif

    deserializeJson(doc, f);
    f.close();
}
