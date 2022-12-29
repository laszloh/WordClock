#pragma once

#include <Arduino.h>
#include <FastLED.h>

#include "Language.h"
#include "esp-hal-log.h"

class LangGer : public Language {

public:
    LangGer() = default;

    virtual void showTime(struct tm *tm) override final {
        FastLED.clear();
        // show the words
        for(auto i = 0; i < 4; i++) {
            const auto dispIndex = pgm_read_byte_near(&displayContents[tm->tm_min / 5][i]);
            if(dispIndex == 255) // we reached the end of the loop, skip remaining bytes
                break;
            showWord(dispIndex);
        }

        if(tm->tm_min / 5 < 5) {
            if(hourFormat12(tm->tm_hour) == 1 && tm->tm_min / 5 == 0) { // Wenn "Ein Uhr", nutze "Ein" statt "Eins" aus hourGroups
                showHour(0);
            } else {
                showHour(hourFormat12(tm->tm_hour));
            }
        } else {
            showHour(hourFormat12(tm->tm_hour + 1));
        }
    }

    virtual void showTestWords() override final {
        showWord(0);
        showWord(1);
        showWord(6);
        showWord(9);
        showHour(10);
        showHour(2);
    }

    static constexpr size_t getLedCount() { return 98; }

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

    static constexpr uint8_t wordGroups[10][2] PROGMEM = {
        // first and last LED used for words, lower one first
        {87, 88}, // "ES"        0
        {90, 92}, // "IST"       1
        {94, 97}, // "FÜNF"      2
        {83, 86}, // "ZEHN"      3
        {66, 72}, // "VIERTEL"   4
        {76, 82}, // "ZWANZIG"   5
        {47, 50}, // "HALB"      6
        {56, 58}, // "VOR"       7
        {61, 64}, // "NACH"      8
        {0, 2}    // "UHR"       9
    };

    static constexpr uint8_t hourGroups[13][2] PROGMEM = {
        // first and last LED used for hours, lower one first
        {13, 15}, // "EIN"      // only used at "ES IST EIN UHR", otherwise index 1-12 = hour
        {13, 16}, // "EINS"
        {52, 55}, // "ZWEI"
        {11, 14}, // "DREI"
        {42, 45}, // "VIER"
        {22, 25}, // "FÜNF"
        {33, 37}, // "SECHS"
        {16, 21}, // "SIEBEN"
        {38, 41}, // "ACHT"
        {4, 7},   // "NEUN"
        {7, 10},  // "ZEHN"
        {30, 32}, // "ELF"
        {25, 29}  // "ZWÖLF"
    };

    static constexpr uint8_t displayContents[12][4] PROGMEM = {
        // first value is current range, minutes() / 5 = 0-11
        {0, 1, 9, 255},   // minute  0 -  4, words 0, 1 and 9  - 255 equals blank/nothing
        {2, 8, 255, 255}, // minute  5 -  9, words 2 and 8
        {0, 1, 3, 8},     // minute 10 - 14
        {4, 8, 255, 255}, // minute 15 - 19
        {0, 1, 5, 8},     // minute 20 - 24
        {2, 7, 6, 255},   // minute 25 - 29
        {0, 1, 6, 255},   // minute 30 - 34
        {2, 8, 6, 255},   // minute 35 - 39
        {0, 1, 5, 7},     // minute 40 - 44
        {4, 7, 255, 255}, // minute 45 - 49
        {0, 1, 3, 7},     // minute 50 - 54
        {2, 7, 255, 255}  // minute 55 - 59
    };
};
