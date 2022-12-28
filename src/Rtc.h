#pragma once

#include <Arduino.h>

#include "config.h"
#include "pins.h"

#ifdef RTC_DS1302

#include <RtcDS1302.h>
#include <ThreeWire.h>
ThreeWire myWire(IO_PIN, CLK_PIN, CE_PIN); // IO/DAT, SCLK, CE/RST
using Rtc = RtcDS1302<ThreeWire>;
inline Rtc rtcInstance() { return Rtc(myWire); }

#elif defined(RTC_DS1307)

#include <RtcDS1307.h>
#include <Wire.h>
using Rtc = RtcDS1307<TwoWire>;
inline Rtc rtcInstance() { return Rtc(Wire); }


#elif defined(RTC_DS3231)

#include <RtcDS3231.h>
#include <Wire.h>
using Rtc = RtcDS3231<TwoWire>;
inline Rtc rtcInstance() { return Rtc(Wire); }

#else
// using no rtc
#pragma warning "No RTC defined"
#define NORTC

#include <RtcDateTime.h>

// dummy class
class Rtc {
public:
    void Begin() { }

    bool IsDateTimeValid() { return false; }
    bool GetIsRunning() { return false; };
    void SetDateTime(const RtcDateTime&) { }

    RtcDateTime GetDateTime() {
        RtcDateTime ret;
        ret.InitWithEpoch32Time(time(nullptr));
        return ret;
    }
};

inline Rtc rtcInstance() { return Rtc(); }

#endif