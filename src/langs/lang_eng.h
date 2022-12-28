#pragma once

#include <Arduino.h>
#include <FastLED.h>

#include "Language.h"

class LangEng : public Language {

public:
    LangEng() = default;

    virtual void showTime(struct tm *tm) override final {
        // show the words
        for(auto i = 0; i < 4; i++) {
            const auto dispIndex = pgm_read_byte_near(&displayContents[tm->tm_min / 5][i]);
            if(dispIndex == 255) // we reached the end of the loop, skip remaining bytes
                break;
            showWord(dispIndex);
        }

        if(tm->tm_min / 5 < 7)
            showHour(hourFormat12(tm->tm_hour));
        else
            showHour(hourFormat12(tm->tm_hour + 1));
    }

    virtual void showTestWords() override final {
        showWord(0);
        showWord(1);
        showWord(6);
        showWord(8);
        showWord(10);
        showHour(12);
    }

    static constexpr size_t getLedCount() { return 100; }


private:
    void showWord(uint8_t w) {
        if(w >= sizeof(wordGroups))
            return;
        const uint8_t startLED = pgm_read_byte_near(&wordGroups[w][0]);
        const uint8_t endLED = pgm_read_byte_near(&wordGroups[w][1]);

        CRGBSet subLed(leds, startLED, endLED);
        subLed = markerHSV;
    }

    void showHour(uint8_t h) {
        if(h >= sizeof(hourGroups))
            return;
        const uint8_t startLED = pgm_read_byte_near(&hourGroups[h][0]);
        const uint8_t endLED = pgm_read_byte_near(&hourGroups[h][1]);

        CRGBSet subLed(leds, startLED, endLED);
        subLed = markerHSV;
    }

    static constexpr uint8_t wordGroups[11][2] PROGMEM = {
        // first and last LED used for words, lower one first
        {95, 96}, // "IT"           0
        {98, 99}, // "IS"           1
        {78, 81}, // "FIVE"         2
        {68, 70}, // "TEN"          3
        {88, 94}, // "QUARTER"      4
        {72, 77}, // "TWENTY"       5
        {72, 81}, // "TWENTYFIVE"   6
        {84, 87}, // "HALF"         7
        {62, 63}, // "TO"           8
        {63, 66}, // "PAST"         9
        {0, 5}    // "O´CLOCK"     10
    };

    static constexpr uint8_t hourGroups[13][2] PROGMEM = {
        // first and last LED used for hours, lower one first
        {255, 255}, // dummy - so we can use index 1-12 for hours 1-12 instead of index 0-11
        {45, 47},   // "ONE"
        {47, 49},   // "TWO"
        {51, 55},   // "THREE"
        {36, 39},   // "FOUR"
        {7, 10},    // "FIVE"
        {42, 44},   // "SIX"
        {27, 31},   // "SEVEN"
        {55, 59},   // "EIGHT"
        {24, 27},   // "NINE"
        {33, 35},   // "TEN"
        {16, 21},   // "ELEVEN"
        {11, 16}    // "TWELVE"
    };

    static constexpr uint8_t displayContents[12][6] PROGMEM = {
        // first value is current range, minutes() / 5 = 0-11
        {0, 0, 1, 10, 255, 255},  // minute  0 -  4, words 0, 1 and 10  - 255 equals blank/nothing
        {1, 2, 9, 255, 255, 255}, // minute  5 -  9, words 2 and 9
        {2, 0, 1, 3, 9, 255},     // minute 10 - 14
        {3, 4, 9, 255, 255, 255}, // minute 15 - 19
        {4, 0, 1, 5, 9, 255},     // minute 20 - 24
        {5, 6, 9, 255, 255, 255}, // minute 25 - 29
        {6, 0, 1, 7, 9, 255},     // minute 30 - 34
        {7, 6, 8, 255, 255, 255}, // minute 35 - 39
        {8, 0, 1, 5, 8, 255},     // minute 40 - 44
        {9, 4, 8, 255, 255, 255}, // minute 45 - 49
        {10, 0, 1, 3, 8, 255},    // minute 50 - 54
        {11, 2, 8, 255, 255, 255} // minute 55 - 59
    };
};
