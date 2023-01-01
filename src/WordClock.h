#pragma once

#include <FastLED.h>
#include <WiFiManager.h>

#include "Language.h"
#include "Rtc.h"
#include "Settings.h"
#include "config.h"

class WordClock {
public:
    WordClock() = default;

    void begin();

    void loop();

    void adjustClock(int8_t hours);
    void adjustInternalTime(time_t newTime) const;

    void setBrightness(bool preview = false);
    void setPalette(bool preview = false);

    void printDebugTime();

    void showSetup(WiFiManager *wm);
    void showReset();
    static void timeUpdate(bool sntp);

private:
    void colorOutput(bool nightMode);

    LangImpl lang;
    CRGBArray<LangImpl::getLedCount()> leds;
    Rtc rtc{rtcInstance()};
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
