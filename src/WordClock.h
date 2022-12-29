#pragma once

#include <FastLED.h>

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
    void setBrightness(Brightness brightness);
    void setColorPalette(uint8_t palette);

    size_t getColorPaletteSize() const { return colorPalettes.size(); }

    void printTime();

    void latchAlarmflags() { rtc.LatchAlarmsTriggeredFlags(); }
    static void wakeupCallback();

private:
    static void timeUpdate(bool sntp);
    static const std::array<CRGBPalette16, 7> colorPalettes;

    void adjustInternalTime(time_t newTime) const;
    void colorOutput(bool nightMode);

    LangImpl lang;
    CRGBArray<LangImpl::getLedCount()> leds;
    Rtc rtc{rtcInstance()};
    uint8_t lastMinute;
    bool wakeup{false};

    // coloring stuff
    bool forceNightMode{false};
    const CHSV nightHSV{CHSV(0, 255, 70)};

    CRGBPalette16 currentPalette;
    uint8_t startColor{0};
    static constexpr uint8_t colorOffset = 8;
};

extern WordClock wordClock;
