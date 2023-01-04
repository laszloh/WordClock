#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <coredecls.h> // settimeofday_cb()
#include <sys/time.h>  // struct timeval
#include <time.h>      // time() ctime()

#include "c++23.h"

#include "Settings.h"
#include "WordClock.h"
#include "esp-hal-log.h"
#include "genTimezone.h"

extern "C" int settimeofday(const struct timeval* tv, const struct timezone* tz);

WordClock wordClock;

/**
 * This routine turns off the I2C bus and clears it
 * on return SCA and SCL pins are tri-state inputs.
 * You need to call Wire.begin() after this to re-enable I2C
 * This routine does NOT use the Wire library at all.
 *
 * returns 0 if bus cleared
 *         1 if SCL held low.
 *         2 if SDA held low by slave clock stretch for > 2sec
 *         3 if SDA held low after 20 clocks.
 */
static int I2C_ClearBus() {
#if defined(TWCR) && defined(TWEN)
    TWCR &= ~(_BV(TWEN)); // Disable the Atmel 2-Wire interface so we can control the SDA and SCL pins directly
#endif

    pinMode(SDA, INPUT_PULLUP); // Make SDA (data) and SCL (clock) pins Inputs with pullup.
    pinMode(SCL, INPUT_PULLUP);

    bool SCL_LOW = (digitalRead(SCL) == LOW); // Check is SCL is Low.
    if(SCL_LOW) {                             // If it is held low Arduno cannot become the I2C master.
        return 1;                             // I2C bus error. Could not clear SCL clock line held low
    }

    bool SDA_LOW = (digitalRead(SDA) == LOW); // vi. Check SDA input.
    int clockCount = 20;                      // > 2x9 clock

    while(SDA_LOW && (clockCount > 0)) { //  vii. If SDA is Low,
        clockCount--;
        // Note: I2C bus is open collector so do NOT drive SCL or SDA high.
        pinMode(SCL, INPUT);        // release SCL pullup so that when made output it will be LOW
        pinMode(SCL, OUTPUT);       // then clock SCL Low
        delayMicroseconds(10);      //  for >5us
        pinMode(SCL, INPUT);        // release SCL LOW
        pinMode(SCL, INPUT_PULLUP); // turn on pullup resistors again
        // do not force high as slave may be holding it low for clock stretching.
        delayMicroseconds(10); //  for >5us
        // The >5us is so that even the slowest I2C devices are handled.
        SCL_LOW = (digitalRead(SCL) == LOW); // Check if SCL is Low.
        int counter = 20;
        while(SCL_LOW && (counter > 0)) { //  loop waiting for SCL to become High only wait 2sec.
            counter--;
            delay(100);
            SCL_LOW = (digitalRead(SCL) == LOW);
        }
        if(SCL_LOW) { // still low after 2 sec error
            return 2; // I2C bus error. Could not clear. SCL clock line held low by slave clock stretch for >2sec
        }
        SDA_LOW = (digitalRead(SDA) == LOW); //   and check SDA input again and loop
    }
    if(SDA_LOW) { // still low
        return 3; // I2C bus error. Could not clear. SDA data line held low
    }

    // else pull SDA line low for Start or Repeated Start
    pinMode(SDA, INPUT);  // remove pullup.
    pinMode(SDA, OUTPUT); // and then make it LOW i.e. send an I2C Start or Repeated start control.
    // When there is only one I2C master a Start or Repeat Start has the same function as a Stop and clears the bus.
    /// A Repeat Start is a Start occurring after a Start with no intervening Stop.
    delayMicroseconds(10);      // wait >5us
    pinMode(SDA, INPUT);        // remove output low
    pinMode(SDA, INPUT_PULLUP); // and make SDA high i.e. send I2C STOP control.
    delayMicroseconds(10);      // x. wait >5us
    pinMode(SDA, INPUT);        // and reset pins as tri-state inputs which is the default state on reset
    pinMode(SCL, INPUT);
    return 0; // all ok
}

void WordClock::begin() {
    FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, leds.size()).setCorrection(TypicalSMD5050).setTemperature(DirectSunlight).setDither(1);
    // FastLED.setMaxPowerInVoltsAndMilliamps(5, LED_PWR_LIMIT);
    FastLED.clear(true);
    FastLED.show();

    lang.assign(leds, leds.size());

    setBrightness();
    setPalette();

    settimeofday_cb(WordClock::timeUpdate);

    I2C_ClearBus();

    // start the RTC
    rtc.Begin();
    // set local clock from rtc if date seems valid
    if(rtc.IsDateTimeValid()) {
        log_d("Settings system time from RTC");
        time_t rtcTime = rtc.GetDateTime().Epoch32Time();
        adjustInternalTime(rtcTime);
    } else {
        // try to fix the rtc, NTP will take care of the rest (or not ¯\_(ツ)_/¯ )
        log_e("RTC is in an invalid state, resetting it!");
        RtcDateTime backup(2020, 01, 01, 00, 00, 00);
        rtc.SetDateTime(backup);
        log_d("Setting bakup: %d", rtc.LastError());
        rtc.SetIsRunning(true);
        log_d("Setting running: %d", rtc.LastError());
    }

    // // perpare the alarm and the wakeup
    log_d("Arming wakeup alarm");
    pinMode(RTCINT_PIN, INPUT);
    rtc.Enable32kHzPin(false);
    rtc.SetSquareWavePin(DS3231SquareWavePin_ModeAlarmTwo);
    DS3231AlarmTwo alarm(0, 0, 0, DS3231AlarmTwoControl::DS3231AlarmTwoControl_OncePerMinute);
    rtc.SetAlarmTwo(alarm);
    rtc.LatchAlarmsTriggeredFlags();
    // prepareAlarm();

    mode = Mode::init;

    // set the time zone (we do not care that we set it twice, if ntp is armed)
    setTZ(timezones[settings.timezone][1]);

    // start the NTP
    if(settings.ntpEnabled)
        configTime(timezones[settings.timezone][1], settings.ntpServer.c_str());

    // Give now a chance to the settimeofday callback,
    // because it is *always* deferred to the next yield()/loop()-call.
    yield();
}

void WordClock::loop() {
    bool updateOutput = false;
    const time_t now = time(nullptr);
    struct tm tm;

    localtime_r(&now, &tm);
    bool nightMode = isNightmode(tm);

    // update the LEDs
    lang.showTime(&tm);
    setBrightness();
    setPalette();

    switch(mode) {
        case Mode::init:
            updateOutput = true;

            mode = Mode::running;
            break;

        case Mode::running:
            if(tm.tm_min != lastMinute) {
                updateOutput = true;
                lastMinute = tm.tm_min;
            }
            break;

        case Mode::setup:
        case Mode::wifi_setup: {

            // override palette
            currentPalette = data::Red_p;
            nightMode = false;

            static CEveryNMillis blink(500);
            if(blink) {
                static bool blank = false;
                if(blank)
                    FastLED.clear();
                blank = !blank;
                updateOutput = true;
            }
        } break;
    }

    // adjust internal time to RTC daily (or if time delta get higher than 1 Minute)
    const uint16_t delta = std::abs(now - rtc.GetDateTime().Epoch32Time());
    static CEveryNHours syncTime(24);
    if(syncTime || delta > 60) {
        log_d("syncing to RTC; delta=%d", delta);
        adjustInternalTime(rtc.GetDateTime().Epoch32Time());
    }

    if(preview)
        previewMode = false;

    // change the color
    startColor += 20;

    // color the leds
    if(updateOutput || previewMode)
        colorOutput(nightMode);
}

void WordClock::colorOutput(bool nightMode) {
    // log_d("Coloring output (nightmode %d)", nightMode);
    if(nightMode) {
        FastLED.setDither(0);
        FastLED.setBrightness(255);
        // colorize all leds in a dark red
        for(CRGB& px : leds) {
            if(px)
                px = nightHSV;
        }
    } else {
        FastLED.setDither(1);
        uint8_t i = 0;
        for(CRGB& px : leds) {
            if(px)
                px = ColorFromPalette(currentPalette, startColor + i * colorOffset, 255);
            i++;
        }
    }
    FastLED.show();
    FastLED.show();
}

bool WordClock::isNightmode(const struct tm& tm) const {
    const auto start = settings.nmStartTime;
    const auto end = settings.nmEndTime;

    return settings.nmEnable && (start < tm || end > tm || forceNightMode);
}

void WordClock::setPalette(bool force) {
    currentPalette = *data::colorPalettes[settings.palette];

    if(force)
        lastMinute = -1;
}


void WordClock::setBrightness(bool force) {
    constexpr std::array brightnessValues = {120, 200, 255};

    if(force)
        lastMinute = -1;

#ifdef NIGHTMODE
    if(settings.brightness == Brightness::night)
        forceNightMode = settings.nmEnable;
    else
#endif
    {
        const auto newBrightness = BrightnessToIndex(settings.brightness);
        FastLED.setBrightness(brightnessValues[newBrightness]);
        forceNightMode = false;
    }
}

constexpr int roundUp(const int numToRound, const int multiple) { return ((numToRound + multiple - 1) / multiple) * multiple; }

void WordClock::prepareAlarm() {
    const time_t now = time(nullptr);
    struct tm tm;

    localtime_r(&now, &tm);
    const bool nightMode = isNightmode(tm);

    // setup next wake event, dependent on the state of night mode
    if(!nightMode) {
        // wake up every minute
        rtc.SetAlarmTwo(DS3231AlarmTwo(0, 0, 0, DS3231AlarmTwoControl::DS3231AlarmTwoControl_OncePerMinute));
        rtc.LatchAlarmsTriggeredFlags();
        return;
    }

    int nextWakeup = roundUp(tm.tm_min + 1, 5);
    if(nextWakeup >= 60)
        nextWakeup = 0;
    log_v("Next wakeup at %d minutes", nextWakeup);

    rtc.SetAlarmTwo(DS3231AlarmTwo(0, 0, nextWakeup, DS3231AlarmTwoControl::DS3231AlarmTwoControl_MinutesMatch));
    rtc.LatchAlarmsTriggeredFlags();
}

void WordClock::printDebugTime() {
    log_d("----------------------------");
    const time_t now = time(nullptr);
    const RtcDateTime rtcNow = rtc.GetDateTime();
    struct tm tm;

    char buf[64];

    gmtime_r(&now, &tm);
    strftime(buf, sizeof(buf), "%F %T", &tm);
    log_d("UTC:   %s", buf);

    localtime_r(&now, &tm);
    strftime(buf, sizeof(buf), "%F %T", &tm);
    log_d("Local: %s", buf);

    log_d("RTC:   %04d-%02d-%02d %02d:%02d:%02d", rtcNow.Year(), rtcNow.Month(), rtcNow.Day(), rtcNow.Hour(), rtcNow.Minute(), rtcNow.Second());
    log_d("----------------------------");
}

void WordClock::setSetup(WiFiManager*) { wordClock.mode = Mode::wifi_setup; }

void WordClock::setRunning() {
    wordClock.setBrightness();
    wordClock.setPalette();
    wordClock.mode = Mode::running;
}

void WordClock::showReset() {
    log_v("Resetting settings");
    currentPalette = data::Red_p;
    FastLED.setBrightness(255);

    for(int i = 0; i < 10; i++) {
        // blink reset text
        FastLED.clear();
        if(i % 2)
            lang.showReset();
        colorOutput(false);
        delay(500);
    }
}

void WordClock::adjustInternalTime(time_t newTime) const {
    timeval tv = {newTime, 0};
    settimeofday(&tv, nullptr);
}

void WordClock::adjustClock(int8_t hours) {
    if(rtc.IsDateTimeValid()) {
        RtcDateTime adjust = rtc.GetDateTime();
        adjust += hours * 60 * 60;
        rtc.SetDateTime(adjust);
        adjustInternalTime(adjust.Epoch32Time());
        colorOutput(false);
    }
}

void WordClock::timeUpdate(bool sntp) {
    // we got an sntp update
    if(sntp) {
        // update rtc with new values
        RtcDateTime newVal;
        newVal.InitWithEpoch32Time(time(nullptr));
        wordClock.rtc.SetDateTime(newVal);
    }
}

uint32_t sntp_update_delay_MS_rfc_not_less_than_15000() {
    uint32_t syncMillis = settings.syncInterval * 60 * 1000;
    return syncMillis;
}
