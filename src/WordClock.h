#pragma once

#include <FastLED.h>

#include "Language.h"
#include "Rtc.h"
#include "config.h"

class WordClock {
public:
    WordClock() = default;

    void begin();

    void loop();

    void adjustClock(int8_t hours);

private:
    static void timeUpdate(bool sntp);

    void adjustInternalTime(time_t newTime) const;
    void colorOutput();

    LangImpl lang;
    CRGBArray<LangImpl::getLedCount()> leds;
    Rtc rtc{rtcInstance()};
    uint8_t lastSecond;
};

extern WordClock wordClock;