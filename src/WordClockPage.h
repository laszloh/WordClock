#pragma once
#include <Arduino.h>
#include <WiFiManager.h>
#include <time.h>

#include "Settings.h"
#include "esp-hal-log.h"


class WordClockPage {
public:
    WordClockPage() = default;

    void begin(WiFiManager *wm);

    void loop();

private:
    void bindServerRequests();
    void handleRoute();
    void handleValues();

    WiFiManager *wm;
};

extern WordClockPage wordClockPage;