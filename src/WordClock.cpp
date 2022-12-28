#include "WordClock.h"
#include <Arduino.h>
#include <coredecls.h> // settimeofday_cb()
#include <sys/time.h>  // struct timeval
#include <time.h>      // time() ctime()

#include "Settings.h"
#include "esp-hal-log.h"
#include "genTimezone.h"

extern "C" int settimeofday(const struct timeval* tv, const struct timezone* tz);

WordClock wordClock;

void WordClock::begin() {
    FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, leds.size()).setCorrection(TypicalSMD5050).setTemperature(DirectSunlight).setDither(1);
    FastLED.setMaxPowerInVoltsAndMilliamps(5, LED_PWR_LIMIT);
    FastLED.clear(true);
    FastLED.show();

    settimeofday_cb(WordClock::timeUpdate);

    // start the RTC
    rtc.Begin();
    // set local clock from rtc if date seems valid
    if(rtc.IsDateTimeValid()) {
        log_d("Settings system time from RTC");
        time_t rtcTime = rtc.GetDateTime().Epoch32Time();
        adjustInternalTime(rtcTime);
    }

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
    time_t now = time(nullptr);
    struct tm tm;

    localtime_r(&now, &tm);

    // update the LEDs
    static CEveryNMillis updateLed(250);
    if(updateLed)
        lang.showTime(&tm);

    if(tm.tm_sec != lastSecond) {
        // color the leds
        colorOutput();
    }
}

void WordClock::colorOutput() {

}

void WordClock::adjustInternalTime(time_t newTime) const {
        timeval tv = {newTime, 0};
        settimeofday(&tv, nullptr);
}

void WordClock::adjustClock(int8_t hours) {
    if(rtc.IsDateTimeValid()) {
        RtcDateTime adjust = rtc.GetDateTime() + hours * 60 * 60;
        rtc.SetDateTime(adjust);
        adjustInternalTime(adjust.Epoch32Time());
        colorOutput();
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