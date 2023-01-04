#pragma once

#include <FastLED.h>
#include <WiFiManager.h>

#include "Language.h"
#include "Rtc.h"
#include "Settings.h"
#include "config.h"

class WordClock {
public:
    enum class Mode : uint8_t { init = 0, running, setup, wifi_setup };

    WordClock() = default;

    void begin();

    void loop();

    void adjustClock(int8_t hours);
    void adjustInternalTime(time_t newTime) const;
    void getTimeFromRtc() { adjustInternalTime(rtc.GetDateTime().Epoch32Time()); }

    void setBrightness(bool preview = false);
    void setPalette(bool preview = false);

    void printDebugTime();
    void latchAlarmflags() { rtc.LatchAlarmsTriggeredFlags(); }

    static void setSetup(WiFiManager *);
    static void setRunning();
    void showReset();
    static void timeUpdate(bool sntp);

    const Mode &getMode() const { return mode; }
    void prepareAlarm();

private:
    void colorOutput(bool nightMode = false);
    bool isNightmode(const struct tm &tm) const;

    LangImpl lang;
    CRGBArray<LangImpl::getLedCount()> leds;
    Rtc rtc{rtcInstance()};
    Mode mode{Mode::init};

    uint8_t lastMinute;

    bool previewMode{false};
    CEveryNSeconds preview{CEveryNBSeconds(2)};

    // coloring stuff
    bool forceNightMode{false};
    const CHSV nightHSV{CHSV(0, 255, 70)};

    CRGBPalette16 currentPalette;
    uint8_t startColor{0};
    static constexpr uint8_t colorOffset = 8;
};

extern WordClock wordClock;
