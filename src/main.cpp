
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <FastLED.h>
#include <LittleFS.h>
#include <WiFiManager.h>

#include "c++23.h"

#include "Button.h"
#include "config.h"
#include "esp-hal-log.h"
#include "pins.h"

#include "Settings.h"
#include "WordClock.h"
#include "WordClockPage.h"

constexpr int serialBaud = 115200;
constexpr SerialConfig serialConfig = SERIAL_8N1;
constexpr SerialMode serialMode = SERIAL_FULL;

inline void setupSerial() { Serial.begin(serialBaud, serialConfig, serialMode); }

Button buttonA;
Button buttonB;

WiFiManager wm;

void buttonAPressed() {
    if(buttonB.pressed()) {
        // start wifi manager
    } else {
        // cycle brightness
        settings.cycleBrightness();
    }
}

void buttonBPressed() {
    if(buttonA.pressed())
        return;

    // cycle palette
    settings.cyclePalette();
}

void buttonALongPress() {
    if(buttonB.longPress()) {
        // start clock setup
    } else {
        // adjust clock by +1 hour
    }
}

void buttonBLongPress() {
    if(buttonA.longPress())
        return;

    // adjust hour by -1 hour
    wordClock.adjustClock(-1);
}

void setup() {
    setupSerial();

    log_i(SKETCHNAME " starting up...");
    log_i("Clock type: " CLOCKNAME);
    log_i("Configured RTC: " RTCNAME);
    log_i("LED power limit: %d mA", LED_PWR_LIMIT);
#ifdef LW_ENG
    log_i("Language: English");
#endif
#ifdef LW_GER
    log_i("Language: German");
#endif
    log_i("Configured for ESP8266");
    log_i("WiFi enabled");
    log_i("NTP enabled");
    log_i("Timezones enabled");
    log_i("AutoDST enabled");
#ifdef FASTFORWARD
    log_w("!! FASTFORWARD defined !!");
#endif

#ifdef AUTOBRIGHTNESS
    log_i("Time based autoBrightness enabled, LDR using pin: %d", pinLDR);
    pinMode(pinLDR, INPUT);
#endif

#ifdef NIGHTMODE
    log_i("Time based night mode enabled");
#endif

    if(!LittleFS.begin()) {
        log_w("File system failed to mount. Formatting...");
        bool ret = LittleFS.format();
        ret &= LittleFS.begin();
        if(!ret) {
            log_e("failed to format & mount file system!");
            while(1) {
                yield();
            }
        }
    }
    log_i("Filesystem mounted");

    if(settings.loadSettings())
        log_i("Settings loaded");
    else {
        log_i("default values loaded");
        settings.saveSettings();
    }

    // boot up the word clock
    wordClock.begin();
    log_i("Word Clock started");

    // set up buttons
    buttonA.begin(BUTA_PIN);
    buttonA.attachClickCallback(buttonAPressed);
    buttonA.attachLogPressStop(buttonALongPress);

    buttonB.begin(BUTB_PIN);
    buttonB.attachClickCallback(buttonBPressed);
    buttonB.attachLogPressStop(buttonBLongPress);

    // check if we should reset our settings
    if(buttonA.pressedRaw() && buttonB.pressedRaw()) {
        wordClock.showReset();
        delay(5000);
        wm.resetSettings();
        ESP.restart();
    }

    // boot up the wifi and the wifi manager
    wordClockPage.begin(&wm);
    wm.setAPCallback(std::bind(&WordClock::showSetup, wordClock, std::placeholders::_1));
    wm.autoConnect("WordClock Setup");
}

void loop() {
    wordClock.loop();
    settings.loop();
    buttonA.loop();
    buttonB.loop();

    static CEveryNSeconds debugHeap(10);
    if(debugHeap)
        log_d("Heap: %d", ESP.getFreeHeap());

    static CEveryNSeconds printTime(10);
    if(printTime)
        wordClock.printDebugTime();

    // static CEveryNSeconds simulate(5);
    // if(simulate) {
    //     static bool firstButton = false;

    //     log_d("Simulation button %c press", (firstButton) ? 'A' : 'B');
    //     if(firstButton)
    //         buttonAPressed();
    //     else
    //         buttonBPressed();
    //     // firstButton = !firstButton;
    // }

    // wordClock.latchAlarmflags();
    // while(!digitalRead(RTCINT_PIN)){
    //     log_d("wait for INT-HIGH");
    //     delay(1);
    // }
    // log_d("preparing for sleep");
    // wifi_station_disconnect(); // not needed
    // wifi_set_opmode(NULL_MODE);
    // wifi_fpm_set_sleep_type(LIGHT_SLEEP_T);
    // wifi_fpm_open();
    // gpio_pin_wakeup_enable(GPIO_ID_PIN(RTCINT_PIN), GPIO_PIN_INTR_LOLEVEL);
    // wifi_fpm_set_wakeup_cb(WordClock::wakeupCallback);
    // wifi_fpm_do_sleep(0xFFFFFFFF);
    // delay(100);

    // log_d("woke up");
    // wordClock.printTime();
}