#pragma once

#include <Arduino.h>
#include <FastLED.h>
#include "c++23.h"

#include "config.h"

class Language {
public:
    Language() = default;

    void assign(CRGB *leds, size_t size) {
        this->leds = leds;
        this->size = size;
    }

    virtual void showTime(struct tm *tm) = 0;
    virtual void showTestWords() = 0;

protected:
    const CHSV markerHSV{CHSV(128, 96, 1)};

    CRGB *leds;
    size_t size;

    int hourFormat12(int hour) { // the hour for the given time in 12 hour format
        if(hour == 0)
            return 12; // 12 midnight
        else if(hour > 12)
            return hour - 12;
        else
            return hour;
    }

};

#ifdef LW_GER

#include "langs/lang_ger.h"
using LangImpl = LangGer;

#elif defined(LW_ENG)

#include "langs/lang_eng.h"
using LangImpl = LangEng;

#endif
