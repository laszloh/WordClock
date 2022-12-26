#include <Arduino.h>
#include "FS.h"
#include <LittleFS.h>
#include <sntp.h>
#include <stdlib.h>

#include "Settings.h"
#include "esp-hal-log.h"


namespace Settings {

constexpr const char *cfgFile = "/config/wordclock.json";

void init() {
    if(!LittleFS.begin()) {
        log_e("failed to open littlefs");
        LittleFS.format();
        LittleFS.begin();
        LittleFS.mkdir("/config");
    }
    File cf = LittleFS.open(cfgFile, "r");
    StaticJsonDocument<2048> doc;

    if(cf) {
        DeserializationError error = deserializeJson(doc, cf);
        if(error) {
            log_e("Deserializationerror: %s (%d)", error.c_str(), error.code());
            log_e("Loading default values...");
        }
    } else
        log_e("File not found. Loading default values...");
    JsonVariant obj = doc;

    serializeJsonPretty(doc, Serial);

    WordClock::loadPerfs(obj);
    NTP::loadPerfs(obj);
    TZ::loadPerfs(obj);
    log_d("Settings loaded");
}

void save() {
log_v("");
    File cf = LittleFS.open(cfgFile, "w");
log_v("");
    StaticJsonDocument<2048> doc;
log_v("");
    JsonVariant set = doc;
log_v("");

    WordClock::savePerfs(set);
log_v("");
    TZ::savePerfs(set);
log_v("");
    NTP::savePerfs(set);
log_v("");
    serializeJson(doc, Serial);
    serializeJson(doc, cf);
log_v("");
    cf.close();
log_v("");
}

} // namespace Settings

namespace Settings::WordClock {

int brightness = 1;
int palette = 1;

void loadPerfs(JsonVariant &obj) {
    brightness = obj["brightness"] | 2;
    palette = obj["palette"] | 1;
}
void savePerfs(JsonVariant &obj) {
    obj["brightness"] = brightness;
    obj["palette"] = palette;
}

bool getBrightness() { return brightness; }
void setBrightness(int newVal) { brightness = newVal; }

bool getPalette() { return palette; }
void setPalette(int newVal) { palette = newVal; }

} // namespace Settings::WordClock

namespace Settings::TZ {

int timezone = TZ_Etc_UTC;

void loadPerfs(JsonVariant &obj) { setTimezone(obj["timezone"] | TZ_Etc_UTC); }

void savePerfs(JsonVariant &obj) { obj["timezone"] = timezone; }

void setTimezone(int tz) {
    if(tz >= timezoneSize)
        return;

    log_d("tz: %d", tz);
    log_d("zone: %s", timezones[timezone][1]);

    timezone = tz;
    setTZ(timezones[timezone][1]);
}


int getTimezone() { return timezone; }

} // namespace Settings::TZ


namespace Settings::NTP {

bool enabled = true;
String ntpServer = "europe.pool.ntp.org";
int syncInterval = 720;

void loadPerfs(JsonVariant &obj) {
    setEnabled(obj["ntp"] | true);
    setServer(obj["ntp-server"] | "europe.pool.ntp.org");
    setSyncInterval(obj["ntp-update"] | 720);
}

void savePerfs(JsonVariant &obj) {
    obj["ntp"] = enabled;
    obj["ntp-server"] = ntpServer;
    obj["ntp-update"] = syncInterval;
}

void updateNtp() {
    if(enabled) {
        configTime(TZ::timezones[TZ::timezone][1], ntpServer.c_str());
    } else {
        sntp_stop();
    }
}

bool getEnabled() { return enabled; }
void setEnabled(const bool en) {
    enabled = en;
    updateNtp();
}


String getServer() { return ntpServer; }
void setServer(const String server) {
    ntpServer = server;
    updateNtp();
}

size_t getSyncInterval() { return syncInterval; }
void setSyncInterval(size_t interval) {
    syncInterval = interval;
    updateNtp();
}

} // namespace Settings::NTP
