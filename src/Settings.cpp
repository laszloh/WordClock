#include "FS.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

#include "Settings.h"
#include "esp-hal-log.h"
#include "genTimezone.h"

Settings settings;

void convertFromJson(JsonVariantConst src, Settings::Time &t) {
    t.hour = src["hour"];
    t.minute = src["minute"];
}

void convertToJson(const Settings::Time &t, JsonVariant dst) {
    dst["hour"] = t.hour;
    dst["minute"] = t.minute;
}

bool canConvertFromJson(JsonVariantConst src, const Settings::Time &) { return src["hour"].is<uint8_t>() && src["minte"].is<uint8_t>(); }

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

    brightness = doc["brightness"] | 1;
    palette = doc["palette"] | 0;

    timezone = doc["timezone"] | TZ_Names::TZ_Europe_Vienna;

    ntpEnabled = doc["ntp-enabled"] | true;
    ntpServer = doc["ntp-server"] | "europe.pool.ntp.org";
    syncInterval = doc["ntp-interval"] | 720;

#ifdef NIGHTMODE
    nmEnable = doc["nm-endable"] | false;
    nmStartTime = doc["nm-start"] | Time{20, 00};
    nmEndTime = doc["nm-end"] | Time{10, 00};
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