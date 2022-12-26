#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

#include "c++23.h"

extern int settimeofday(const struct timeval* tv, const struct timezone* tz);

namespace Settings {

void init();
void save();

} // namespace Settings

namespace Settings::WordClock {

void loadPerfs(JsonVariant &obj);
void savePerfs(JsonVariant &obj);

bool getBrightness();
void setBrightness(int newVal);

bool getPalette();
void setPalette(int newVal);

} // namespace Settings::WordClock


namespace Settings::TZ {

#include "genTimezone.h"

void loadPerfs(JsonVariant &obj);
void savePerfs(JsonVariant &obj);

void setTimezone(int tz);
int getTimezone();

} // namespace Settings::TZ


namespace Settings::NTP {

void loadPerfs(JsonVariant &obj);
void savePerfs(JsonVariant &obj);

void updateNtp();

bool getEnabled();
void setEnabled(const bool en);

String getServer();
void setServer(const String server);

size_t getSyncInterval();
void setSyncInterval(size_t interval);

} // namespace Settings::NTP
