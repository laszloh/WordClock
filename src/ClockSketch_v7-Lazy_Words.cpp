/* -[ClockSketch v7.2]----------------------------------------------------------------------------------------
   https://www.instructables.com/ClockSketch-V7-Part-I/

   Modified for Lazy Words


   Arduino UNO/Nano/Pro Mini (AtMega328, 5V, 16 MHz), DS3231 RTC

   September 2022 - Daniel Cikic

   Serial Baud Rates:
   Arduino: 57600
   ESP8266: 74880
-------------------------------------------------------------------------------------------------------------- */

#include <Arduino.h>
#include <coredecls.h> // optional settimeofday_cb() callback to check on server
#include <sys/time.h>
#include <time.h> // for time() ctime() ...
#include <sntp.h>

#if 0

// comment below to disable serial in-/output and free some RAM
#define DEBUG

// Lazy Words, uncomment only the one to be used
// #define LW_ENG  // english
#define LW_GER // german

// ESP8266 - uncomment to compile this sketch for ESP8266 1.0 / ESP8266, make sure to select the proper board
// type inside the IDE! This mode is NOT supported and only experimental!
#define ESP8266
// #define LIGHT_SLEEP

// ESP32 - uncomment to compile this sketch for ESP8266 1.0 / ESP32, make sure to select the proper board
// type inside the IDE! This mode is NOT supported and only experimental!
// #define ESP32

// useWiFi - enable WiFi support, WPS setup only! If no WPS support is available on a router check settings
// further down, set useWPS to false and enter ssid/password there --- no wps on lazy words!!
#define USEWIFI

// useNTP - enable NTPClient, requires ESP8266 and USEWIFI. This will also enforce AUTODST.
// Configure a ntp server further down below!
#define USENTP

// RTC selection - uncomment the one you're using, comment all others and make sure pin assignemts for
// DS1302 are correct in the parameters section further down!
// #define RTC_DS1302
// #define RTC_DS1307
#define RTC_DS3231

// autoDST - uncomment to enable automatic DST switching, check Time Change Rules below!
// #define AUTODST

// FADING - uncomment to enable fading effects for dots/digits, other parameters further down below
#define FADING

// autoBrightness - uncomment to enable automatic brightness adjustments by using a photoresistor/LDR
// #define AUTOBRIGHTNESS

// FastForward will speed up things and advance time, this is only for testing purposes!
// Disables AUTODST, USENTP and USERTC.
// #define FASTFORWARD

/* ----------------------------------------------------------------------------------------------------- */

#include <EEPROM.h> // required for reading/saving settings to eeprom

/* Start RTC config/parameters--------------------------------------------------------------------------
   Check pin assignments for DS1302 (SPI), others are I2C (A4/A5 on Arduino by default)
   Currently all types are using the "Rtc by Makuna" library                                             */
#ifdef RTC_DS1302
#include <RtcDS1302.h>
#include <ThreeWire.h>
ThreeWire myWire(7, 6, 8); // IO/DAT, SCLK, CE/RST
RtcDS1302<ThreeWire> Rtc(myWire);
#define RTCTYPE "DS1302"
#define USERTC
#endif

#ifdef RTC_DS1307
#include <RtcDS1307.h>
#include <Wire.h>
RtcDS1307<TwoWire> Rtc(Wire);
#define RTCTYPE "DS1307"
#define USERTC
#endif

#ifdef RTC_DS3231
#include <RtcDS3231.h>
#include <Wire.h>
RtcDS3231<TwoWire> Rtc(Wire);
#define RTCTYPE "DS3231"
#define RTCINTPIN 2
#define USERTC
#endif

#if !defined(USERTC)
#pragma message "No RTC selected, check definitions on top of the sketch!"
#endif
/* End RTC config/parameters---------------------------------------------------------------------------- */

/* Start WiFi config/parameters------------------------------------------------------------------------- */
#ifdef USEWIFI
constexpr bool useWPS = false; // set to false to disable WPS and use credentials below - no wps on lazy words!
constexpr const char *wifiSSID = "maWhyFhy";
constexpr const char *wifiPWD = "5up3r1337r0xX0r!";
#endif
/* End WiFi config/parameters--------------------------------------------------------------------------- */

/* Start NTP config/parameters--------------------------------------------------------------------------
   Using NTP will enforce autoDST, so check autoDST/time zone settings below!                            */
#ifdef USENTP
/* I recommend using a local ntp service (many routers offer them), don't spam public ones with dozens
   of requests a day, get a rtc! ^^                                                                    */
#define NTPHOST "europe.pool.ntp.org"
String ntpServer = NTPHOST;
// #define NTPHOST "192.168.2.1"
#ifndef AUTODST
#define AUTODST
#endif
#endif
/* End NTP config/parameters---------------------------------------------------------------------------- */

/* Start autoBrightness config/parameters -------------------------------------------------------------- */
constexpr uint8_t upperLimitLDR
    = 120; // everything above this value will cause max brightness (according to current level) to be used (if it's higher than this)
constexpr uint8_t lowerLimitLDR = 80; // everything below this value will cause minBrightness to be used
constexpr uint8_t minBrightness = 60; // anything below this avgLDR value will be ignored
constexpr bool nightMode = false; // nightmode true -> if minBrightness is used, colorizeOutput() will use a single color for everything, using HSV
constexpr uint8_t nightColor[2] = {0, 70}; // hue 0 = red, fixed brightness of 70, https://github.com/FastLED/FastLED/wiki/FastLED-HSV-Colors
constexpr float factorLDR
    = 1.0; // try 0.5 - 2.0, compensation value for avgLDR. Set dbgLDR true & define DEBUG and watch the serial monitor. Looking...
constexpr const bool dbgLDR = false; // ...for values roughly in the range of 120-160 (medium room light), 40-80 (low light) and 0 - 20 in the dark
#ifdef ESP8266
constexpr uint8_t pinLDR = 0; // LDR connected to A0 (ESP8266 only offers this one)
#elif defined(ESP32)
constexpr uint8_t pinLDR = 12;
#else
constexpr uint8_t pinLDR = 1;  // LDR connected to A1 (in case somebody flashes this sketch on arduino and already has an ldr connected to A1)
#endif
constexpr uint8_t intervalLDR = 75; // read value from LDR every 75ms (most LDRs have a minimum of about 30ms - 50ms)
uint16_t avgLDR = 0;                // we will average this value somehow somewhere in readLDR();
uint16_t lastAvgLDR = 0;            // last average LDR value we got
/* End autoBrightness config/parameters ---------------------------------------------------------------- */

#define SKETCHNAME "ClockSketch v7.2"
#define CLOCKNAME "Lazy Words v1"

#ifdef DEBUG

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define LOG(fmt, ...) Serial.printf_P((PGM_P)PSTR(fmt), ##__VA_ARGS__)

#else

#define LOG(...)                                                                                                                                     \
    do {                                                                                                                                             \
        (void)0;                                                                                                                                     \
    } while(0)

#endif

/* Start button config/pins----------------------------------------------------------------------------- */
#ifdef ESP8266
constexpr uint8_t buttonA = 13; // momentary push button, 1 pin to gnd, 1 pin to d7 / GPIO_13
constexpr uint8_t buttonB = 12; // momentary push button, 1 pin to gnd, 1 pin to d5 / GPIO_14
#elif defined(ESP32)
constexpr uint8_t buttonA = 3; // momentary push button, 1 pin to gnd, 1 pin to d3
constexpr uint8_t buttonB = 4; // momentary push button, 1 pin to gnd, 1 pin to d4
#else
constexpr uint8_t buttonA = 3; // momentary push button, 1 pin to gnd, 1 pin to d3
constexpr uint8_t buttonB = 4; // momentary push button, 1 pin to gnd, 1 pin to d4
#endif
/* End button config/pins------------------------------------------------------------------------------- */

/* Start basic appearance config------------------------------------------------------------------------ */
uint8_t colorMode = 0; // different color modes, setting this to anything else than zero will overwrite values written to eeprom -- not on Lazy Words!
uint16_t colorSpeed = 600;                           // controls how fast colors change (ms)
constexpr bool colorPreview = true;                  // true = preview selected palette/colorMode using "8" on all positions for 3 seconds
constexpr uint8_t colorPreviewDuration = 3;          // duration in seconds for previewing palettes/colorModes if colorPreview is enabled/true
constexpr bool reverseColorCycling = true;           // true = reverse color movements
constexpr uint8_t brightnessLevels[3]{80, 130, 240}; // 0 - 255, brightness Levels (min, med, max) - index (0-2) will be saved to eeprom
uint8_t brightness = brightnessLevels[0];            // default brightness if none saved to eeprom yet / first run
#ifdef FADING
constexpr uint8_t fadePixels = 2; // fade pixels, 0 = disabled, 1 = only fade out pixels turned off, 2 = fade old out and fade new in
constexpr uint8_t fadeDelay = 30; // milliseconds between each fading step, 5-25 should work okay-ish
#endif
/* End basic appearance config-------------------------------------------------------------------------- */

/* End of basic config/parameters section */

/* End of feature/parameter section, unless changing advanced things/modifying the sketch there's absolutely nothing to do further down! */

/* library, wifi and ntp stuff depending on above config/parameters */
#ifdef ESP8266
#if defined(USENTP) && !defined(USEWIFI) // enforce USEWIFI when USENTP is defined
#define USEWIFI
#pragma warning "USENTP without USEWIFI, enabling WiFi"
#endif
#ifdef USEWIFI
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#endif
#elif defined(ESP32)
#if defined(USENTP) && !defined(USEWIFI) // enforce USEWIFI when USENTP is defined
#define USEWIFI
#pragma warning "USENTP without USEWIFI, enabling WiFi"
#endif
#ifdef USEWIFI
#include <AsyncTCP.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#endif
#endif

#ifdef USEWIFI

#include "WordClockParameter.h"
#include <WiFiManager.h>

WiFiManager wm;
#define WM_PORTAL_NAME "WordClock"

#endif


#ifdef DEBUG

#include "esp-hal-log.h"
#include <SoftwareSerial.h>

constexpr int serialBaud = 74880;
constexpr SerialConfig serialConfig = SERIAL_8N1;
constexpr SerialMode serialMode = SERIAL_FULL;

int log_printf(PGM_P fmt, ...) {
    va_list arg;
    char temp[64];
    char *buffer = temp;

    va_start(arg, fmt);
    size_t len = vsnprintf_P(temp, sizeof(temp), fmt, arg);
    va_end(arg);
    if(len > sizeof(temp) - 1) {
        buffer = new char[len + 1];
        if(!buffer)
            return 0;
        va_start(arg, fmt);
        vsnprintf_P(buffer, len + 1, fmt, arg);
        va_end(arg);
    }
    len = Serial.write((const uint8_t *)buffer, len);
    if(buffer != temp)
        delete[] buffer;
    return len;
}

void setupSerial() { Serial.begin(serialBaud, serialConfig, serialMode); }

#else

#define log_printf(...)                                                                                                                              \
    do {                                                                                                                                             \
        (void)0;                                                                                                                                     \
    } while(0)

#define setupSerial()                                                                                                                                \
    do {                                                                                                                                             \
        (void)0;                                                                                                                                     \
    } while(0)

#endif

/* end library stuff */

/* setting feature combinations/options */
#if defined(FASTFORWARD) || defined(CUSTOMHELPER)
bool firstLoop = true;
#ifdef USERTC
#undef USERTC
#endif
#ifdef USEWIFI
#undef USEWIFI
#endif
#ifdef USENTP
#undef USENTP
#endif
#ifdef AUTODST
#undef AUTODST
#endif
#endif
/* setting feature combinations/options */

/* Start of FastLED/clock stuff */
#ifdef ESP8266
#define FASTLED_RAW_PIN_ORDER // this means we'll be using the raw esp8266 pin order -> GPIO_12, which is d6 on ESP8266
#define LED_PIN 15            // led data in connected to GPIO_12 (d6/ESP8266)
#elif defined(ESP32)
#define FASTLED_RAW_PIN_ORDER // this means we'll be using the raw esp8266 pin order -> GPIO_12, which is d6 on ESP8266
#define LED_PIN 12            // led data in connected to GPIO_12 (d6/ESP8266)
#else
#define FASTLED_ALLOW_INTERRUPTS 0 // AVR + WS2812 + IRQ = https://github.com/FastLED/FastLED/wiki/Interrupt-problems
#define LED_PIN 6                  // led data in connected to d6 (arduino)
#endif

#define LED_PWR_LIMIT 800 // Power limit in mA (voltage is set in setup() to 5v)

#ifdef LW_ENG
#define LED_COUNT 100 // change if using additional leds to indicate am/pm!
#endif

#ifdef LW_GER
#define LED_COUNT 98
#endif

#include <FastLED.h>

const CHSV markerHSV(96, 255, 100); // this color will be used to "flag" leds for coloring later on while updating the leds
CRGBArray<LED_COUNT> leds;
CRGBPalette16 currentPalette;

// start clock specific config/parameters

// ---- english ----

#ifdef LW_ENG
constexpr uint8_t wordGroups[11][2] PROGMEM = {
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
    {0, 5}    // "O??CLOCK"     10
};

constexpr uint8_t hourGroups[13][2] PROGMEM = {
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

constexpr uint8_t displayContents[12][6] PROGMEM = {
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
#endif

// ---- german ----

#ifdef LW_GER
const uint8_t wordGroups[10][2] PROGMEM = {
    // first and last LED used for words, lower one first
    {87, 88}, // "ES"        0
    {90, 92}, // "IST"       1
    {94, 97}, // "F??NF"      2
    {83, 86}, // "ZEHN"      3
    {66, 72}, // "VIERTEL"   4
    {76, 82}, // "ZWANZIG"   5
    {47, 50}, // "HALB"      6
    {56, 58}, // "VOR"       7
    {61, 64}, // "NACH"      8
    {0, 2}    // "UHR"       9
};

const uint8_t hourGroups[13][2] PROGMEM = {
    // first and last LED used for hours, lower one first
    {13, 15}, // "EIN"      // only used at "ES IST EIN UHR", otherwise index 1-12 = hour
    {13, 16}, // "EINS"
    {52, 55}, // "ZWEI"
    {11, 14}, // "DREI"
    {42, 45}, // "VIER"
    {22, 25}, // "F??NF"
    {33, 37}, // "SECHS"
    {16, 21}, // "SIEBEN"
    {38, 41}, // "ACHT"
    {4, 7},   // "NEUN"
    {7, 10},  // "ZEHN"
    {30, 32}, // "ELF"
    {25, 29}  // "ZW??LF"
};

const uint8_t displayContents[12][6] PROGMEM = {
    // first value is current range, minutes() / 5 = 0-11
    {0, 0, 1, 9, 255, 255},   // minute  0 -  4, words 0, 1 and 9  - 255 equals blank/nothing
    {1, 2, 8, 255, 255, 255}, // minute  5 -  9, words 2 and 8
    {2, 0, 1, 3, 8, 255},     // minute 10 - 14
    {3, 4, 8, 255, 255, 255}, // minute 15 - 19
    {4, 0, 1, 5, 8, 255},     // minute 20 - 24
    {5, 2, 7, 6, 255, 255},   // minute 25 - 29
    {6, 0, 1, 6, 255, 255},   // minute 30 - 34
    {7, 2, 8, 6, 255, 255},   // minute 35 - 39
    {8, 0, 1, 5, 7, 255},     // minute 40 - 44
    {9, 4, 7, 255, 255, 255}, // minute 45 - 49
    {10, 0, 1, 3, 7, 255},    // minute 50 - 54
    {11, 2, 7, 255, 255, 255} // minute 55 - 59
};
#endif

uint8_t clockStatus = 1; // Used for various things, don't mess around with it! 1 = startup
                         // 0 = regular mode, 1 = startup, 9x = setup modes (90, 91, 92, 93...), 80 = color preview mode on Lazy Words

/* these values will be saved to EEPROM:
  0 = index for selected palette
  1 = index for selected brightness level */

/* End of FastLED/clock stuff */
// End clock specific configs/parameters

/* other global variables */
uint8_t btnRepeatCounter = 0; // keeps track of how often a button press has been repeated
bool eepromCommit = false;
/* */

/* -- this is where the fun parts start -------------------------------------------------------------------------------------------------------- */

#if !defined(ESP8266) && !defined(ESP32)
constexpr void yield() { }
#endif

void showWord(uint8_t w);
void showHour(uint8_t h);
void paletteSwitcher();
void brightnessSwitcher();
void printTime();
void displayTime(struct tm *tm);
void colorizeOutput(uint8_t mode);
void pixelFader();
void setupClock();
uint8_t dbgInput();
uint8_t inputButtons();
void syncHelper();
time_t getTimeNTP();
void saveWifiManagerParameters();
void armRTCAlarm();

void eepromSaveString(const int addr, const String &str) {
    EEPROM.write(addr, str.length());
    for(size_t i = 0; i < str.length(); i++)
        EEPROM.write(addr + 1 + i, str[i]);
}

String eepromLoadString(const int addr) {
    const auto len = EEPROM.read(addr);
    char buffer[len + 1] = {0x00};
    for(auto i = 0; i < len; i++)
        buffer[i] = EEPROM.read(addr + 1 + i);
    return String(buffer);
}

void saveEEPROMSettings() { }

void setup() {

    setupSerial();

#ifdef SKETCHNAME
    log_i(SKETCHNAME " starting up...");
#endif
#ifdef CLOCKNAME
    log_i("Clock type: " CLOCKNAME);
#endif
#ifdef RTCTYPE
    log_i("Configured RTC: " RTCTYPE);
#endif
    log_i("LED power limit: %d mA", LED_PWR_LIMIT);
    log_i("Total LED count: %d", LED_COUNT);
#ifdef AUTODST
    log_i("autoDST enabled");
#endif
#ifdef LW_ENG
    log_i("Language: English");
#endif
#ifdef LW_GER
    log_i("Language: German");
#endif
#if defined(ESP8266) || defined(ESP32)
    log_i("Configured for ESP8266 / ESP32");
#ifdef USEWIFI
    log_i("WiFi enabled");
#endif
#ifdef USENTP
    log_i("NTP enabled, NTPHOST: " NTPHOST);
#endif
#else
    log_i("Configured for Arduino");
#endif
#ifdef FASTFORWARD
    log_w("!! FASTFORWARD defined !!");
#endif

#ifdef AUTOBRIGHTNESS
    log_i("autoBrightness enabled, LDR using pin: %d", pinLDR);
    pinMode(pinLDR, INPUT);
#endif

    FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, leds.size()).setCorrection(TypicalSMD5050).setTemperature(DirectSunlight).setDither(1);
    FastLED.setMaxPowerInVoltsAndMilliamps(5, LED_PWR_LIMIT);
    FastLED.clear(true);
    FastLED.show();

    pinMode(buttonA, INPUT_PULLUP);
    pinMode(buttonB, INPUT_PULLUP);

#ifdef DEBUG
    if(digitalRead(buttonA) == LOW || digitalRead(buttonB) == LOW) {
        if(digitalRead(buttonA) == LOW) {
            log_d("buttonA is LOW / pressed - check wiring!");
        }
        if(digitalRead(buttonB) == LOW) {
            log_d("buttonB is LOW / pressed - check wiring!");
            log_d("Lazy Words test display starting in 3 seconds...");
            unsigned long testStart = millis();
            while(millis() - testStart < 3000)
                yield();
#ifdef LW_ENG
            showWord(0);
            showWord(1);
            showWord(6);
            showWord(8);
            showWord(10);
            showHour(12);
#endif
#ifdef LW_GER
            showWord(0);
            showWord(1);
            showWord(6);
            showWord(9);
            showHour(10);
            showHour(2);
#endif
            for(uint8_t i = 0; i < LED_COUNT; i++) {
                if(leds[i])
                    leds[i].setHSV(0, 160, 160);
            }
            FastLED.show();
            log_d("...done, reset/power cycle to restart.");
            while(1)
                yield();
        }
    }
#endif

#if defined(ESP8266) || defined(ESP32) // if building for ESP8266...
    EEPROM.begin(512);
#ifdef USEWIFI // ...and if using WiFi.....
    log_d("Starting up WiFi...");
    bool saveConfig = false;

    WordClockParams::begin(&wm);
    wm.autoConnect(WM_PORTAL_NAME);

    // we finished with the portal
    if(saveConfig) {
        // changes were made
        saveEEPROMSettings();
    } else {
        // load config
        brightness = EEPROM.read(1);
        ntpServer = eepromLoadString(2);
    }
#endif
#endif

#ifdef USERTC
    Rtc.Begin();
    Wire.setClock(400000);
#ifdef LIGHT_SLEEP
    Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeAlarmTwo);
#endif
    if(!Rtc.IsDateTimeValid())
        Rtc.SetDateTime(RtcDateTime(2020, 1, 1, 0, 0, 0));
    if(!Rtc.GetIsRunning())
        Rtc.SetIsRunning(true);

    log_i("RTC.begin(), 2 second safety delay before doing any read/write actions!");
    unsigned long tmp_time = millis();
    while(millis() - tmp_time < 2000)
        yield();
    log_i("RTC initialized");
#else
    log_w("No RTC defined!");
#endif

    paletteSwitcher();
    brightnessSwitcher();

#ifdef FASTFORWARD
    setTime(11, 58, 00, 31, 8, 2022); // h, m, s, d, m, y to set the clock to when using FASTFORWARD
#endif

#ifdef USENTP
    log_d("Starting ntp with server: %s", Settings::NTP::getServer().c_str());
    Settings::NTP::updateNtp();
    syncHelper();
#endif

    clockStatus = 0; // change from 1 (startup) to 0 (running mode)

    printTime();
    log_i("Setup done");
    log_i("------------------------------------------------------");
}

/* MAIN LOOP */

void loop() {
    static uint8_t lastInput = 0;           // != 0 if any button press has been detected
    static uint8_t lastSecondDisplayed = 0; // This keeps track of the last second when the display was updated (HH:MM and HH:MM:SS)
    static bool doUpdate = false;   // Update led content whenever something sets this to true. Coloring will always happen at fixed intervals!
    time_t sysTime = time(nullptr); // if no rtc is defined, get local system time

    constexpr uint8_t refreshDelay = 5; // refresh leds every 5ms

    if(lastInput != 0) {           // If any button press is detected...
        if(btnRepeatCounter < 1) { // execute short/single press function(s)
            log_d("%d (short press)", lastInput);
            if(lastInput == 1) // short press button A
                brightnessSwitcher();
            else if(lastInput == 2) { // short press button B
                paletteSwitcher();
                if(colorPreview) {
                    unsigned long paletteChanged = millis();
                    uint16_t colorSpeedBak = colorSpeed;
                    colorSpeed = 3;
                    log_d("Palette changed, previewing");
                    clockStatus = 80;
                    while(millis() - paletteChanged <= colorPreviewDuration * 1000) {
                        displayTime(localtime(&sysTime));
                        colorizeOutput(colorMode);
                        FastLED.show();
                        FastLED.clear();
                        yield();
                    }
                    log_d("Preview finsihed");
                    colorSpeed = colorSpeedBak;
                    clockStatus = 0;
                }
            }
            if(lastInput == 3) { // short press button A + button B
            }
        } else if(btnRepeatCounter > 8) { // execute long press function(s)...
            btnRepeatCounter = 1;         // ..reset btnRepeatCounter to stop this from repeating
            log_d("%d (long press)", lastInput);
            if(lastInput == 1) { // long press button A, set hour - 1
#ifdef USERTC
                log_d("Waiting for RTC...");
                // waiting for RTC
                while(Rtc.GetDateTime().Second() == lastSecondDisplayed)
                    yield();
                Rtc.SetDateTime((Rtc.GetDateTime() - 3600));
                log_d("Time changed by -1 hour");
#else
                log_w("No action without an RTC!");
#endif
                FastLED.clear();
                displayTime(localtime(&sysTime));
                colorizeOutput(colorMode);
                FastLED.show();
#ifdef FADING
                pixelFader();
#endif
            }
            if(lastInput == 2) { // long press button B, set hour + 1
#ifdef USERTC
                log_d("Waiting for RTC...");
                // waiting for RTC
                while(Rtc.GetDateTime().Second() == lastSecondDisplayed)
                    yield();
                Rtc.SetDateTime((Rtc.GetDateTime() + 3600));
                log_d("Time changed by +1 hour");
#else
                log_w("No action without an RTC!");
#endif
                FastLED.clear();
                displayTime(localtime(&sysTime));
                colorizeOutput(colorMode);
                FastLED.show();
#ifdef FADING
                pixelFader();
#endif
            }
            if(lastInput == 3) { // long press button A + button B
#ifdef USEWIFI                   // if USEWIFI is defined and...
                bool saveConfig = false;
                // wm.setSaveParamsCallback([&saveConfig]() { saveConfig = true; });
                wm.startConfigPortal(WM_PORTAL_NAME);
                if(saveConfig) {
                    log_d("Saving new setting...");
                    saveEEPROMSettings();
                    log_d("Rebooting");
                    ESP.reset();
                }

#else // if USEWIFI is not defined...
                FastLED.clear();
                FastLED.show();
                setupClock(); // start time setup
#endif
            }
            while(digitalRead(buttonA) == LOW || digitalRead(buttonB) == LOW) { // wait until buttons are released again
                static CEveryNMillis refreshLeds(50);
                if(refreshLeds) { // Refresh leds every 50ms to give optical feedback
                    colorizeOutput(colorMode);
                    FastLED.show();
                }
                yield();
            }
        }
    }

#ifdef FASTFORWARD // if FASTFORWARD is defined...
    static CEveryNMillis fastForwardDelay(100);
    if(fastForwardDelay) // ...and 100ms have passed...
        adjustTime(15);  // ...add 15 seconds to current time
#endif

#ifdef LIGHT_SLEEP
    // we always check the RTC, since we were woken up here
    constexpr bool checkRTC = true;
#else
    static CEveryNMillis checkRTC(50);
#endif
    if(checkRTC) {
#ifdef USERTC
        sysTime = Rtc.GetDateTime().Epoch32Time();
#else
        sysTime = time(nullptr);
#endif
#ifndef LIGHT_SLEEP
        struct tm tm;
        gmtime_r(&sysTime, &tm);
        if(lastSecondDisplayed != tm.tm_sec)
#endif
            doUpdate = true;
    }

    if(doUpdate) { // this will update the led array if doUpdate is true because of a new second from the rtc
        timeval v = {sysTime, 0};
        settimeofday(&v, nullptr);        // sync system time to rtc every second
        FastLED.clear();                  // 1A - clear all leds...
        displayTime(localtime(&sysTime)); // 2A - output sysTime/rtcTime to the led array..
        lastSecondDisplayed = localtime(&sysTime)->tm_sec;
        doUpdate = false;
        if(lastSecondDisplayed % 20 == 0) {
            printTime();
        }
#ifdef USENTP // if NTP is enabled, resync to ntp server
        syncHelper();
#endif
    }

    colorizeOutput(colorMode); // 1C, 2C, 3C...colorize the data inside the led array right now...
#ifdef AUTOBRIGHTNESS
    static CEveryNMillis updateLDR(intervalLDR);

    if(updateLDR) {
        readLDR();                          // ...call readLDR();
        if(abs(avgLDR - lastAvgLDR) >= 5) { // if avgLDR has changed for more than +/- 5 update lastAvgLDR
            lastAvgLDR = avgLDR;
            FastLED.setBrightness(avgLDR);
        }
    }
#endif
#ifdef FADING
    pixelFader();
#endif

    WordClockParams::loop();

    static CEveryNMillis displayRefresh(refreshDelay);
    if(displayRefresh)
        FastLED.show();

    lastInput = inputButtons();

    // #if defined(LIGHT_SLEEP) && defined(USERTC)
    //     armRTCAlarm();

    //     constexpr uint32_t timeout = UINT32_MAX;
    //     WiFi.mode(WIFI_OFF);
    //     delay(100);
    //     wifi_fpm_set_sleep_type(LIGHT_SLEEP_T);
    //     wifi_fpm_open();
    //     wifi_enable_gpio_wakeup(RTCINTPIN, GPIO_PIN_INTR_LOLEVEL);
    //     wifi_enable_gpio_wakeup(buttonA, GPIO_PIN_INTR_LOLEVEL);
    //     wifi_enable_gpio_wakeup(buttonB, GPIO_PIN_INTR_LOLEVEL);
    //     wifi_fpm_do_sleep(timeout);
    //     wifi_fpm_close();
    //     WiFi.mode(WIFI_STA);
    //     WiFi.begin();
    // #endif
}

void armRTCAlarm() {
    // constexpr int interval = 5;

    // const RtcDateTime sysTime = Rtc.GetDateTime().Epoch32Time();
    // auto mins = ((sysTime.Minute() + interval / 2) / interval) * interval;
    // if(mins >= 60)
    //     mins -= 60;

    // arm clock for every minute wakeups
    const DS3231AlarmTwo alarm(0, 0, 0, DS3231AlarmTwoControl::DS3231AlarmTwoControl_OncePerMinute);
    Rtc.SetAlarmTwo(alarm);
}

/* */

void showWord(uint8_t w) {
    if(w >= sizeof(wordGroups))
        return;
    const uint8_t startLED = pgm_read_byte_near(&wordGroups[w][0]);
    const uint8_t endLED = pgm_read_byte_near(&wordGroups[w][1]);
    leds(startLED, endLED) = markerHSV;
}

void showHour(uint8_t h) {
    if(h >= sizeof(hourGroups))
        return;
    const uint8_t startLED = pgm_read_byte_near(&hourGroups[h][0]);
    const uint8_t endLED = pgm_read_byte_near(&hourGroups[h][1]);
    leds(startLED, endLED) = markerHSV;
}

#ifdef FADING
void fadePixel(uint8_t i, uint8_t amount, uint8_t fadeType) {
    /* this will check if the first led of a given segment is lit and if it is, will fade by
       amount using fadeType. fadeType is important because when fading things in that where
       off previously we must avoid setting them black at first - hence fadeLightBy instead
       of fadeToBlack.  */
    if(leds[i]) {
        if(fadeType)
            leds[i].fadeLightBy(amount);
        else
            leds[i].fadeToBlackBy(amount);
    }
}

void pixelFader() {
    constexpr uint8_t fadeSteps = 15; // steps used to fade in or out

    if(fadePixels == 0)
        return;
    static unsigned long firstRun = 0;             // time when a change has been detected and fading starts
    static boolean active = false;                 // will be used as a flag when to do something / fade pixels
    static bool previousPixels[LED_COUNT] = {0};   // all the pixels lit after the last run
    static uint8_t changedPixels[LED_COUNT] = {0}; // used to store the differences -> 1 = led has been turned off, fade out, 2 = was off, fade in
    if(!active) {                                  // this will check if....
        firstRun = millis();
        uint8_t idx = 0;
        for(const auto px : leds) {
            if(px && bool(px) != previousPixels[idx]) {
                active = true;
                changedPixels[idx] = (px) ? 2 : 1;
            }
            idx++;
        }
    } else { // this part is executed once a change has been detected....
        static uint8_t counter = 1;
        static CEveryNMillis lastFadeStep(fadeDelay);
        for(uint8_t i = 0; i < leds.size(); i++) { // redraw pixels that have turned off, so we can fade them out...
            if(changedPixels[i] == 1)
                leds[i] = markerHSV;
        }
        colorizeOutput(colorMode); // colorize again after redraw, so colors keep consistent
        for(uint8_t i = 0; i < LED_COUNT; i++) {
            if(changedPixels[i] == 1)                           // 1 - pixel has turned on, this one has to be faded in
                fadePixel(i, counter * (255.0 / fadeSteps), 0); // fadeToBlackBy, pixels supposed to be off/fading out
            else if(changedPixels[i] == 2) {                    // 2 - pixel has turned off, this one has to be faded out
                if(fadePixels == 2)
                    fadePixel(i, 255 - counter * (255.0 / fadeSteps), 1); // fadeLightBy, pixels supposed to be on/fading in
            }
        }

        if(lastFadeStep)
            counter++;

        if(counter > fadeSteps) { // done with fading, reset variables...
            counter = 1;
            active = false;
            uint8_t idx = 0;
            for(const auto px : leds) {
                previousPixels[idx] = px;
                idx++;
            }
            memset(changedPixels, 0, LED_COUNT);
            log_d("pixel fading sequence took %d ms", millis() - firstRun);
        }
    }
}
#endif

#ifdef AUTOBRIGHTNESS
void readLDR() { // read LDR value 5 times and write average to avgLDR
    static uint8_t runCounter = 1;
    static uint16_t tmp = 0;
    uint8_t readOut = map(analogRead(pinLDR), 0, 1023, 0, 250);
    tmp += readOut;
    if(runCounter == 5) {
        avgLDR = (tmp / 5) * factorLDR;
        tmp = 0;
        runCounter = 0;
        if(dbgLDR)
            log_d("avgLDR value: %d", avgLDR);
        if(avgLDR < minBrightness)
            avgLDR = minBrightness;
        if(avgLDR > brightness)
            avgLDR = brightness;
        if(avgLDR >= upperLimitLDR && avgLDR < brightness)
            avgLDR = brightness; // if avgLDR is above upperLimitLDR switch to max current brightness
        if(avgLDR <= lowerLimitLDR)
            avgLDR = minBrightness; // if avgLDR is below lowerLimitLDR switch to minBrightness
        if(dbgLDR)
            log_d("avgLDR adjusted to: %d", avgLDR);
    }
    runCounter++;
}
#endif

void setupClock() {
    /* This sets time and date (if AUTODST is defined) on the clock/rtc */
    clockStatus = 90; // clockStatus 9x = setup, relevant for other functions/coloring
    // do nothing until both buttons are released to avoid accidental inputs right away
    while(digitalRead(buttonA) == LOW || digitalRead(buttonB) == LOW)
        yield();
    struct tm setupTime;    // Create a time element which will be used. Using the current time would
    setupTime.tm_hour = 12; // give some problems (like time still running while setting hours/minutes)
    setupTime.tm_min = 0;   // Setup starts at 12 (12 pm) (utc 12 if AUTODST is defined)
    setupTime.tm_sec = 0;   //
    setupTime.tm_mday = 18; // date settings only used when AUTODST is defined, but will set them anyways
    setupTime.tm_mon = 8;   // see above
    setupTime.tm_year = 52; // current year - 1970 (2022 - 1970 = 52) -- date cannot be set on Lazy Words manually!
    uint8_t lastInput = 0;
    // minutes
    while(lastInput != 2) {
        clockStatus = 93;
        if(lastInput == 1) {
            if(setupTime.tm_min < 55) {
                setupTime.tm_min += 5;
            } else {
                setupTime.tm_min = 0;
                if(setupTime.tm_hour < 23) {
                    setupTime.tm_hour++;
                } else {
                    setupTime.tm_hour = 0;
                }
            }
            char buffer[64];
            strftime(buffer, sizeof(buffer), "%T", &setupTime);
            log_d("%s", buffer);
        }
        displayTime(&setupTime);
        colorizeOutput(colorMode);
        FastLED.show();
        lastInput = inputButtons();
    }
    lastInput = 0;
    log_d("HH:MM:SS -> %02d:%02d:%02d", setupTime.tm_hour, setupTime.tm_min, setupTime.tm_sec);
#ifdef USERTC
    time_t rtcTime = mktime(&setupTime);
    RtcDateTime rtcDateTime;
    rtcDateTime.InitWithEpoch32Time(rtcTime);
    Rtc.SetDateTime(rtcDateTime);
    log_d("RTC time set %lld", rtcTime);
#endif
    timeval tv = {rtcTime, 0};
    settimeofday(&tv, nullptr);
    printTime();
    clockStatus = 0;
    LOG("setupClock done");
}

void colorizeOutput(uint8_t mode) {
    /* So far showWord()/showHour() only set some leds inside the array to values from "markerHSV" but we haven't updated
       the leds yet using FastLED.show(). This function does the coloring of the right now single colored but "insivible"
       output. This way color updates/cycles aren't tied to updating display contents                                      */
    static uint8_t startColor = 0;
    uint8_t colorOffset = 4; // different offsets result in quite different results, depending on the amount of leds inside each segment...
                             // ...so it's set inside each color mode if required
    if(clockStatus == 80)    // 80 -> color preview cylcing after switching in loop()
        colorOffset = 20;

    // mode 0 = check every segment if it's lit and assign a color based on led #
    if(mode == 0) {
        uint8_t i = 0;
        for(auto i = 0; i < leds.size(); i++) {
            if(leds[i])
                leds[i] = ColorFromPalette(currentPalette, startColor + i * colorOffset, brightness, LINEARBLEND);
        }
        // for(auto px : leds) {
        //     if(px)
        //         px = ColorFromPalette(currentPalette, startColor + i * colorOffset, brightness, LINEARBLEND);
        //     i++;
        // }
        // for ( uint8_t i = 0; i < LED_COUNT; i++ ) {
        //   if ( leds[i] ) leds[i] = ColorFromPalette(currentPalette, startColor + i * colorOffset, brightness, LINEARBLEND);
        // }
    }

    /* clockStatus >= 90 is used for coloring output while in setup mode */
    if(clockStatus >= 90) {
        static boolean blinkFlag = true;
        static unsigned long lastBlink = millis();
        static uint8_t b = brightnessLevels[0];
        if(millis() - lastBlink > 333) { // blink switch frequency, 3 times a second
            if(blinkFlag) {
                blinkFlag = false;
                b = brightnessLevels[1];
            } else {
                blinkFlag = true;
                b = brightnessLevels[0];
            }
            lastBlink = millis();
        } // unset values = red, set value = green, current value = yellow and blinkinkg
        for(uint8_t i = 0; i < LED_COUNT; i++) {
            if(leds[i])
                leds[i].setHSV(0, 255, b);
        }
    }


#ifdef FASTFORWARD
    const uint16_t colorChangeInterval = 30;
#else
    const uint16_t colorChangeInterval = colorSpeed;
#endif
    static CEveryNMillis colorChange(colorChangeInterval);
    colorChange.setPeriod(colorChangeInterval);

    if(colorChange)
        startColor += (reverseColorCycling) ? -1 : +1;

#ifdef AUTOBRIGHTNESS
    if(nightMode && clockStatus == 0) { // nightmode will overwrite everything that has happened so far...
        if(avgLDR <= minBrightness) {
            FastLED.setDither(0);
            for(auto px : leds) {
                if(px)
                    px.setHSV(nightColor[0], 255, nightColor[1]); // and assign nightColor to all lit leds. Default is a very dark red.
            }
        } else
            FastLED.setDither(1);
    }
#endif
}

int hourFormat12(int hour) { // the hour for the given time in 12 hour format
    if(hour == 0)
        return 12; // 12 midnight
    else if(hour > 12)
        return hour - 12;
    else
        return hour;
}

void displayTime(struct tm *tm) {
    if(clockStatus >= 90) // while in setup this will clear the display each time when redrawing
        FastLED.clear();

    for(uint8_t i = 1; i < 6; i++) {
        const auto dispIndex = pgm_read_byte_near(&displayContents[tm->tm_min / 5][i]);
        if(dispIndex == 255) // we reached the end of the loop, skip remaining bytes
            break;
        showWord(dispIndex);
    }

#ifdef LW_ENG
    if(tm->tm_min / 5 < 7)
        showHour(hourFormat12(tm->tm_hour));
    else
        showHour(hourFormat12(tm->tm_hour + 1));
#elif defined(LW_GER)
    if(tm->tm_min / 5 < 5) {
        if(hourFormat12(tm->tm_hour) == 1 && tm->tm_min / 5 == 0) { // Wenn "Ein Uhr", nutze "Ein" statt "Eins" aus hourGroups
            showHour(0);
        } else {
            showHour(hourFormat12(tm->tm_hour));
        }
    } else {
        showHour(hourFormat12(tm->tm_hour + 1));
    }
#endif
}

void paletteSwitcher() {
    /* As the name suggests this takes care of switching palettes. When adding palettes, make sure paletteCount increases
      accordingly.  A few examples of gradients/solid colors by using RGB values or HTML Color Codes  below               */
    uint8_t currentIndex = Settings::WordClock::getPalette();
    switch(currentIndex) {
        case 0:
            currentPalette = CRGBPalette16(CRGB(224, 0, 40), CRGB(8, 0, 244), CRGB(100, 0, 180), CRGB(208, 0, 96));
            break;
        case 1:
            currentPalette = CRGBPalette16(CRGB(224, 16, 0), CRGB(192, 64, 0), CRGB(192, 140, 0), CRGB(240, 40, 0));
            break;
        case 2:
            currentPalette = CRGBPalette16(CRGB::Aquamarine, CRGB::Turquoise, CRGB::Blue, CRGB::DeepSkyBlue);
            break;
        case 3:
            currentPalette = RainbowColors_p;
            break;
        case 4:
            currentPalette = PartyColors_p;
            break;
        case 5:
            currentPalette = CRGBPalette16(CRGB::LawnGreen);
            break;
    }
    log_d("selected palette %d", currentIndex);
}

void brightnessSwitcher() {
    uint8_t currentIndex = Settings::WordClock::getBrightness();
    switch(currentIndex) {
        case 0:
            brightness = brightnessLevels[currentIndex];
            break;
        case 1:
            brightness = brightnessLevels[currentIndex];
            break;
        case 2:
            brightness = brightnessLevels[currentIndex];
            break;
    }
    log_d("selected brightness index %d", currentIndex);
}

uint8_t inputButtons() {
    /* This scans for button presses and keeps track of delay/repeat for user inputs
       Short keypresses will only be returned when buttons are released before repeatDelay
       is reached. This is to avoid constantly sending 1 or 2 when executing a long button
       press and/or multiple buttons.
       Note: Buttons are using pinMode INPUT_PULLUP, so HIGH = not pressed, LOW = pressed! */
    static uint8_t scanInterval = 30;            // only check buttons every 30ms
    static uint16_t repeatDelay = 1000;          // delay in milliseconds before repeating detected keypresses
    static uint8_t repeatRate = 1000 / 7;        // 7 chars per 1000 milliseconds
    static uint8_t minTime = scanInterval * 2;   // minimum time to register a button as pressed
    static unsigned long lastReadout = millis(); // keeps track of when the last readout happened
    static unsigned long lastReturn = millis();  // keeps track of when the last readout value was returned
    static uint8_t lastState = 0;                // button state from previous scan
    uint8_t currentState = 0;                    // button state from current scan
    uint8_t retVal = 0;                          // return value, will be 0 if no button is pressed
    static unsigned long eventStart = millis();  // keep track of when button states are changing
    if(millis() - lastReadout < scanInterval)
        return 0; // only scan for button presses every <scanInterval> ms
    if(digitalRead(buttonA) == LOW)
        currentState += 1;
    if(digitalRead(buttonB) == LOW)
        currentState += 2;
    if(currentState == 0 && currentState == lastState) {
        btnRepeatCounter = 0;
    }
    if(currentState != 0 && currentState != lastState) { // if any button is pressed and different from the previous scan...
        eventStart = millis();                           // ...reset eventStart to current time
        btnRepeatCounter = 0;                            // ...and reset global variable btnRepeatCounter
    }
    if(currentState != 0 && currentState == lastState) { // if same input has been detected at least twice (2x scanInterval)...
        if(millis() - eventStart >= repeatDelay) {       // ...and longer than repeatDelay...
            if(millis() - lastReturn >= repeatRate) {    // ...check for repeatRate...
                retVal = currentState;                   // ...and set retVal to currentState
                btnRepeatCounter++;
                lastReturn = millis();
            } else
                retVal = 0; // return 0 if repeatDelay hasn't been reached yet
        }
    }
    if(currentState == 0 && currentState != lastState && millis() - eventStart >= minTime && btnRepeatCounter == 0) {
        retVal = lastState; // return lastState if all buttons are released after having been pressed for <minTime> ms
        btnRepeatCounter = 0;
    }
    lastState = currentState;
    lastReadout = millis();
    uint8_t serialInput = dbgInput();
    if(serialInput != 0) {
        // Serial.print(F("inputButtons(): Serial input detected: ")); Serial.println(serialInput);
        retVal = serialInput;
    }
    if(retVal != 0)
        LOG("inputButtons(): Return value is: %d - btnRepeatCounter is: %d", retVal, btnRepeatCounter);
    return retVal;
}

// following will only be included if USENTP is defined
#ifdef USENTP

/* This syncs system time to the RTC at startup and will periodically do other sync related
   things, like syncing rtc to ntp time */
void syncHelper() {
    static CEveryNMinutes ntpUpdate(60);
    const auto newPeriode = Settings::NTP::getSyncInterval();
    if(newPeriode != ntpUpdate.getPeriod()) {
        log_d("Setting NTP periode to %ld", newPeriode);
        ntpUpdate.setPeriod(newPeriode);
    }

    if(ntpUpdate || clockStatus == 1) {
        if(clockStatus > 1) {
            log_w("Clock is not running!");
            return;
        }
        if(WiFi.status() != WL_CONNECTED) {
            log_e("No Wifi connection!");
            return;
        }
#ifndef USERTC
#ifndef USENTP
        log_w("No RTC and no NTP configured, nothing to do...");
        return;
#endif
#endif

        time_t ntpTime = 0;
        if(clockStatus == 1) // looks like the sketch has just started running...
            log_d("Initial sync on power up...");
        else
            log_d("Resyncing to NTP...");

        ntpTime = getTimeNTP();
        log_d("NTP result is %lld", ntpTime);
#ifdef USERTC
        RtcDateTime rtcTime = Rtc.GetDateTime(); // get current time from the rtc....
        timeval v = {ntpTime, 0};
        settimeofday(&v, nullptr);

        const auto delta = int(difftime(rtcTime.Epoch32Time(), ntpTime));
        if(ntpTime > 100 && std::abs(delta) > 100) {
            rtcTime.InitWithEpoch32Time(ntpTime);
            Rtc.SetDateTime(rtcTime);
            if(!Rtc.GetIsRunning())
                Rtc.SetIsRunning(true);
        }
#else
        log_d("No RTC configured, using system time");
        log_d("sysTime was %d", now());
        if(ntpTime > 100)
            setTime(ntpTime);
#endif
        log_d("syncHelper done");
    }
}

time_t getTimeNTP() {
    unsigned long startTime = millis();
    time_t timeNTP;
    if(WiFi.status() != WL_CONNECTED)
        log_e("Not connected, WiFi.status is %d", WiFi.status());
    // Sometimes the connection doesn't work right away although status is WL_CONNECTED...
    // ...so we'll wait a moment before causing network traffic
    while(millis() - startTime < 2000)
        yield();
    timeNTP = sntp_get_current_timestamp();
    log_d("getTimeNTP done");
    return timeNTP;
}
#endif
// ---

// functions below will only be included if DEBUG is defined on top of the sketch
void printTime() {
    /* outputs current system and RTC time to the serial monitor, adds autoDST if defined */
    RtcDateTime gmTime;
    gmTime.InitWithEpoch32Time(time(nullptr));
#ifdef USERTC
    RtcDateTime rtcTime = Rtc.GetDateTime();
#endif
    log_d("-----------------------------------");
    log_d("System time is: %02d:%02d:%02d", gmTime.Hour(), gmTime.Minute(), gmTime.Second());
    log_d("System date is: %02d/%02d/%02d (Y/M/D)", gmTime.Year(), gmTime.Month(), gmTime.Day());
#ifdef USERTC
    log_d("RTC time is: %02d:%02d:%02d", rtcTime.Hour(), rtcTime.Minute(), rtcTime.Second());
    log_d("RTC date is: %02d/%02d/%02d (Y/M/D)", rtcTime.Year(), rtcTime.Month(), rtcTime.Day());
#endif
#ifdef AUTODST
    struct tm local;
    time_t gm = gmTime.Epoch32Time();
    localtime_r(&gm, &local);
    log_d("autoDST time is: %02d:%02d:%02d", local.tm_hour, local.tm_min, local.tm_sec);
    log_d("autoDST date is: %02d/%02d/%02d (Y/M/D)", 1900 + local.tm_year, local.tm_mon, local.tm_mday);
#endif
    log_d("-----------------------------------");
}

uint8_t dbgInput() {
#ifdef DEBUG
    /* this catches input from the serial console and hands it over to inputButtons() if DEBUG is defined
        Serial input "7" matches buttonA, "8" matches buttonB, "9" matches buttonA + buttonB */
    if(Serial.available() > 0) {
        uint8_t incomingByte = 0;
        incomingByte = Serial.read();
        if(incomingByte == 52) { // 4 - long press buttonA
            btnRepeatCounter = 10;
            return 1;
        }
        if(incomingByte == 53) { // 5 - long press buttonB
            btnRepeatCounter = 10;
            return 2;
        }
        if(incomingByte == 54) { // 6 - long press buttonA + buttonB
            btnRepeatCounter = 10;
            return 3;
        }
        if(incomingByte == 55)
            return 1; // 7 - buttonA
        if(incomingByte == 56)
            return 2; // 8 - buttonB
        if(incomingByte == 57)
            return 3; // 9 - buttonA + buttonB
    }
#endif
    return 0;
}


#endif