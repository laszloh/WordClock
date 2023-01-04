
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <FastLED.h>
#include <LittleFS.h>
#include <RTCMemory.h>
#include <Schedule.h>
#include <WiFiManager.h>
#include <coredecls.h>

#include "c++23.h"

#include "Button.h"
#include "config.h"
#include "esp-hal-log.h"
#include "pins.h"

#include "Settings.h"
#include "WordClock.h"
#include "WordClockPage.h"

constexpr int serialBaud = 74880;
constexpr SerialConfig serialConfig = SERIAL_8N1;
constexpr SerialMode serialMode = SERIAL_FULL;
constexpr const char *wmProtalName PROGMEM = "WordClock Setup";

struct WiFiState {
    uint32_t crc;
    struct {
        station_config fwconfig;
        ip_info ip;
        ip_addr_t dns[2];
        ip_addr_t ntp[2];
        WiFiMode_t mode;
        uint8_t channel;
        bool persistent;
    } state;
};

struct RtcData {
    WiFiState stateSave;
    time_t now;
};

Button buttonA;
Button buttonB;
WiFiManager wm;
RTCMemory<RtcData> rtcMemory;

inline void setupSerial() { Serial.begin(serialBaud, serialConfig, serialMode); }

void buttonAPressed() {
    if(buttonB.pressed()) {
        // start wifi manager
        WiFi.resumeFromShutdown(rtcMemory.getData()->stateSave);

        wordClock.setSetup(&wm);
        wm.startConfigPortal(wmProtalName);
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

RF_PRE_INIT() { system_phy_set_powerup_option(2); }

void setup() {
    setupSerial();
    rtcMemory.begin();

    WiFi.shutdown(rtcMemory.getData()->stateSave);

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

    // boot up the wifi and the wifi manager
    wordClockPage.begin(&wm);
    wm.setAPCallback(WordClock::setSetup);
    wm.setConfigResetCallback(Settings::resetSettings);
    wm.setSaveConfigCallback([]() { WordClock::setRunning(); });
    wm.setConfigPortalBlocking(false);
    wm.setCountry("JP");
    wm.setBreakAfterConfig(true);

    // check if we should reset our settings
    if(buttonA.pressedRaw() && buttonB.pressedRaw()) {
        log_w("Resetting settings!");
        wordClock.showReset();
        // we delayed already 5s, if the buttons are still pressed, reset
        if(buttonA.pressedRaw() && buttonB.pressedRaw()) {
        wm.resetSettings();
        ESP.restart();
        }
    }

    if(settings.wifiEnable) {
        WiFi.resumeFromShutdown(rtcMemory.getData()->stateSave);

        wm.autoConnect(wmProtalName);
    }
}

void IRAM_ATTR wakeupCallback() {
    wordClock.getTimeFromRtc();
    schedule_function([]() { log_d("Callback"); });
}

void IRAM_ATTR wakeupPinIsrWE() {
    // Wakeup IRQs are available as level-triggered only.
    detachInterrupt(RTCINT_PIN);
    schedule_function([]() { log_d("GPIO wakeup IRQ"); });
}

void loop() {
    wm.process();

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

    // don't go to sleep if we are in a setup mode
    if(wordClock.getMode() != WordClock::Mode::running)
        return;

    wordClock.latchAlarmflags();
    while(!digitalRead(RTCINT_PIN)) {
        log_d("wait for INT-HIGH");
        delay(1);
    }
    log_d("preparing for sleep");
    Serial.flush();

    WiFi.shutdown(rtcMemory.getData()->stateSave);
    wifi_fpm_close();
    wifi_set_opmode(NULL_MODE);
    wifi_fpm_set_sleep_type(LIGHT_SLEEP_T);
    wifi_fpm_open();
    wifi_fpm_set_wakeup_cb(wakeupCallback);
    attachInterrupt(RTCINT_PIN, wakeupPinIsrWE, ONLOW_WE);
    // gpio_pin_wakeup_enable(GPIO_ID_PIN(RTCINT_PIN), GPIO_PIN_INTR_LOLEVEL);
    wifi_fpm_do_sleep(0xFFFFFFFF);
    delay(100);
    // esp_yield();

    if(settings.wifiEnable)
        WiFi.resumeFromShutdown(rtcMemory.getData()->stateSave);

    log_d("woke up");
    wordClock.printDebugTime();
    // wordClock.prepareAlarm();
}