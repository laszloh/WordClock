#pragma once

// LIGHT_SLEEP - uncomment to enable light sleep by usign the RTC alarm to wake up the uC
// #define LIGHT_SLEEP

// RTC selection - uncomment the one you're using, comment all others and make sure pin assignemts for
// DS1302 are correct in the parameters section further down!
// #define RTC_DS1302
// #define RTC_DS1307
#define RTC_DS3231

// FADING - uncomment to enable fading effects for dots/digits, other parameters further down below
#define FADING

// autoBrightness - uncomment to enable automatic brightness adjustments by using a photoresistor/LDR
//#define AUTOBRIGHTNESS

// nightMode - uncomment to enable time based palette change to a dark red
#define NIGHTMODE

// FastForward will speed up things and advance time, this is only for testing purposes!
// Disables AUTODST, USENTP and USERTC.
// #define FASTFORWARD

// LED_PWR_LIMIT - Power limit in mA
#define LED_PWR_LIMIT 800

// Lazy Words, uncomment only the one to be used
// #define LW_ENG  // english
#define LW_GER // german

#define SKETCHNAME "ClockSketch v8.0"
#define CLOCKNAME "Lazy Words v1"
