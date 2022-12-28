
#include "c++23.h"
#include <Arduino.h>
#include <LittleFS.h>

#include "Button.h"
#include "config.h"
#include "esp-hal-log.h"
#include "pins.h"

#include "Settings.h"
#include "WordClock.h"


constexpr int serialBaud = 74880;
constexpr SerialConfig serialConfig = SERIAL_8N1;
constexpr SerialMode serialMode = SERIAL_FULL;

inline void setupSerial() {
    Serial.begin(serialBaud, serialConfig, serialMode);
    delay(2000); // to give the user time to start the serial console
}

Button buttonA;
Button buttonB;

void buttonAPressed() {
    // cycle brightness
}

void buttonALongPress() {
    if(buttonB.pressed()) {
        // start wifi manager
    } else {
        // adjust clock by +1 hour
    }
}

void buttonBPressed() {
    // cycle pattern
}

void buttonBLongPress() {
    // adjust hour by -1 hour
    if(!buttonA.pressed())
        wordClock.adjustClock(-1);
}

void setup() {
    setupSerial();

    log_i(SKETCHNAME " starting up...");
    log_i("Clock type: " CLOCKNAME);
    log_i("Configured RTC: DS3221");
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
    buttonA.loop();
    buttonB.loop();

    wordClock.loop();
}