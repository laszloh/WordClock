
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <LittleFS.h>
#include <user_interface.h>

#include "c++23.h"

#include "Button.h"
#include "config.h"
#include "esp-hal-log.h"
#include "pins.h"

#include "Settings.h"
#include "WordClock.h"

constexpr int serialBaud = 115200;
constexpr SerialConfig serialConfig = SERIAL_8N1;
constexpr SerialMode serialMode = SERIAL_FULL;

inline void setupSerial() {
    Serial.begin(serialBaud, serialConfig, serialMode);
    delay(2000); // to give the user time to start the serial console
}

Button buttonA;
Button buttonB;

void buttonAPressed() {
    if(buttonB.pressed()) {
        // start wifi manager
    } else {
        // cycle brightness
        settings.brightness++;
        settings.saveSettings();
        wordClock.setBrightness(settings.brightness);
    }
}

void buttonBPressed() {
    if(buttonA.pressed())
        return;

    // cycle palette
    settings.palette++;
    if(settings.palette >= wordClock.getColorPaletteSize())
        settings.palette = 0;
    settings.saveSettings();
    wordClock.setColorPalette(settings.palette);
}

void buttonALongPress() {
    if(buttonB.pressed()) {
        // start clock setup
    } else {
        // adjust clock by +1 hour
    }
}

void buttonBLongPress() {
    if(buttonA.pressed())
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

    LittleFSConfig cfg;
    LittleFS.setConfig(cfg);
    LittleFS.begin();
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
}

void loop() {
    wordClock.loop();
    buttonA.loop();
    buttonB.loop();

    static CEveryNSeconds debugHeap(10);
    if(debugHeap)
        log_d("Heap: %d", ESP.getFreeHeap());

    static CEveryNSeconds printTime(5);
    if(printTime)
        wordClock.printTime();

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