#pragma once
#include <Arduino.h>
#include <WiFiManager.h>
#include <time.h>

#include "Settings.h"
#include "esp-hal-log.h"

namespace wc {

namespace WordClockParams {

WiFiManager *_wm;
bool saveRequested;

void bindServerRequests();

void begin(WiFiManager *wm) {
    constexpr const char *menuhtml = "<form action='/custom' method='get'><button>Setup Clock</button></form><br/>";

    _wm = wm;
    wm->setWebServerCallback(bindServerRequests);
    std::vector<const char *> menu = {"wifi", "custom", "info", "param", "sep", "restart", "exit"};
    wm->setMenu(menu);
    wm->setCustomMenuHTML(menuhtml);
    // Settings::init();
}

void loop() {
    if(saveRequested) {
        saveRequested = false;
        // Settings::save();
    }
}

const char *getSystimeStr() {
    static char systime[64];
    struct timeval tv;
    time_t now;
    struct tm *nowtm;

    gettimeofday(&tv, NULL);
    now = tv.tv_sec;
    nowtm = localtime(&now);
    strftime(systime, sizeof(systime), "%FT%T", nowtm);
    log_v("systime: %s", systime);
    return systime;
}

void setSystemTime(const String localTime) {
    // struct tm tm = {0};
    // strptime(localTime.c_str(), "%FT%T", &tm);
    // time_t now = mktime(&tm) - _timezone;
    // log_d("UTC time: %lld", now);
    // timeval tvNow;
    // tvNow.tv_sec = now;
    // tvNow.tv_usec = 0;
    // settimeofday(&tvNow, NULL);
}

void handleRoute() {
    log_d("HTTP] Handle route Custom");

    String TimeConfHTML;
    TimeConfHTML.reserve(28000);
    TimeConfHTML += HTTP_HEAD_START;
    TimeConfHTML.replace(FPSTR(T_v), "Word Clock setup");

    TimeConfHTML += HTTP_SCRIPT;
    TimeConfHTML += F("<script>;window.addEventListener('load', function() { var now = new Date(); "
                      "var offset = now.getTimezoneOffset() * 60000;"
                      "var adjustedDate = new Date(now.getTime() - offset);"
                      "document.getElementById('set-time').value = adjustedDate.toISOString().substring(0,16); });"
                      "</script>");
    TimeConfHTML += HTTP_STYLE;
    TimeConfHTML += F("<style>input[type='checkbox'][name='use-ntp-server']:not(:checked) ~.collapsable{display:none;}"
                      "input[type='checkbox'][name='use-ntp-server']:checked ~.collapsed{display:none;}</style>");
    TimeConfHTML += HTTP_HEAD_END;
    TimeConfHTML += F("<iframe name='dummyframe' id='dummyframe' style='display: none;'></iframe>"
                      "<form action='/save-wc' target='dummyframe' method='POST' novalidate>");
    // const int brightness = Settings::WordClock::getBrightness();
    TimeConfHTML += F("<h1>WordClock Settings</h1>"
                      "<p>Brightness</p>"
                      "<input style='display: inline-block;' type='radio' id='choice1' name='brightness' value='0' ");
    // TimeConfHTML += (brightness == 0) ? "checked" : "";
    TimeConfHTML += F("><label for='choice1'>Low</label><br/>"
                      "<input style='display: inline-block;' type='radio' id='choice2' name='brightness' value='1' ");
    // TimeConfHTML += (brightness == 1) ? "checked" : "";
    TimeConfHTML += F("><label for='choice2'>Medium</label><br/>"
                      "<input style='display: inline-block;' type='radio' id='choice3' name='brightness' value='2' ");
    // TimeConfHTML += (brightness == 2) ? "checked" : "";
    TimeConfHTML += F("><label for='choice3'>high</label><br/>"
                      "<br/>"
                      "<label for='palette'>Color Palette</label>"
                      "<select name='palette' id='palette' class='button'>"
                      "<option value='0' ");
    // const int palette = Settings::WordClock::getPalette();
    // TimeConfHTML += (palette == 0) ? "selected" : "";
    TimeConfHTML += F(">Red-Blue</option>"
                      "<option value='1' ");
    TimeConfHTML += (palette == 1) ? "selected" : "";
    TimeConfHTML += F(">Orange Fire</option>"
                      "<option value='2' ");
    TimeConfHTML += (palette == 2) ? "selected" : "";
    TimeConfHTML += F(">Blue Ice</option>"
                      "<option value='3' ");
    TimeConfHTML += (palette == 3) ? "selected" : "";
    TimeConfHTML += F(">Rainbow</option>"
                      "<option value='4' ");
    TimeConfHTML += (palette == 4) ? "selected" : "";
    TimeConfHTML += F(">Party</option>"
                      "<option value='5' ");
    TimeConfHTML += (palette == 5) ? "selected" : "";
    TimeConfHTML += F(">Green</option>"
                      "</select>"
                      "<h1>Time Settings</h1>"
                      "<label for='timezone'>Time Zone</label>"
                      "<select id='timezone' name='timezone'>");
    // for(size_t i = 0; i < Settings::TZ::timezoneSize; i++) {
    //     bool selected = (Settings::TZ::getTimezone() == i);
    //     TimeConfHTML += "<option value='" + String(i) + "' " + String(selected ? "selected" : "") + ">" + String(FPSTR(Settings::TZ::timezones[i][0]))
    //         + "</option>";
    // }
    TimeConfHTML += F("</select><br><br>"
                      "<label for='use-ntp-server'>Enable NTP Client</label> "
                      " <input value='1' type=checkbox name='use-ntp-server' id='use-ntp-server'");
    // TimeConfHTML += String(Settings::NTP::getEnabled() ? "checked" : "");
    TimeConfHTML += F("><br/>"
                      "<div class='collapsed'>"
                      "<label for='set-time'>Set Time "
                      "<input style=width:auto name='set-time' step='1' id='set-time' type='datetime-local'></div>"
                      "<div class='collapsable'>"
                      "<h2>NTP Client Setup</h2>"
                      "<br><label for='ntp-server'>Server:</label>"
                      "<input type='text' id='ntp-server' name='ntp-server' value='");
    // TimeConfHTML += Settings::NTP::getServer();
    TimeConfHTML += F("'><br>"
                      "<label for='ntp-interval'>Sync interval:</label>"
                      "<select id='ntp-interval' name='ntp-interval'>"
                      "<option value=60>Hourly</option>"
                      "<option value=240>Every 4 hours</option>"
                      "<option value=720 selected>Every 12 hours</option>"
                      "<option value=1440>Daily</option>"
                      "<option value=2880>Every 2 Days</option>"
                      "<option value=10080>Weekly</option>"
                      "</select><br>"
                      "</div>"
                      "<button type=submit>Submit</button>"
                      "</form>");

    TimeConfHTML += HTTP_END;

    log_d("TimeConfHTML: %d", TimeConfHTML.length());

    _wm->server->send(200, "text/html", TimeConfHTML.c_str());
}

void handleValues() {
    log_d("[HTTP] handle route Values");
    auto &srv = _wm->server;

    // WordClock
    if(srv->hasArg("brightness")) {
        const String strBrightness = srv->arg("brightness");
        log_v("brightness: %s", strBrightness.c_str());
        int brightness = std::max(int(strBrightness.toInt()), 2);
        // Settings::WordClock::setBrightness(brightness);
    }

    if(srv->hasArg("palette")) {
        const String strPalette = srv->arg("palette");
        log_v("palette: %s", strPalette.c_str());
        int palette = std::max(int(strPalette.toInt()), 5);
        // Settings::WordClock::setPalette(palette);
    }

    // Timezones
    if(srv->hasArg("timezone")) {
        // const String strTz = srv->arg("timezone");
        // log_v("Timezone: %s", strTz.c_str());
        // size_t tzId = strTz.toInt();
        // if(tzId >= Settings::TZ::timezoneSize)
        //     tzId = Settings::TZ::TZ_Names::TZ_Etc_UTC;
        // Settings::TZ::setTimezone(tzId);
    }

    // NTP
    const String useNtpServer = srv->arg("use-ntp-server");
    log_v("UseNtpServer: %s", useNtpServer.c_str());
    const bool NTPEnabled = useNtpServer.toInt() == 1;
    // Settings::NTP::setEnabled(NTPEnabled);

    if(NTPEnabled) {
        // get the rest of the args

        // NTP server
        String NTPServer = "pool.ntp.org";
        if(srv->hasArg("ntp-server")) {
            String NTPServer = srv->arg("ntp-server");
            log_v("NTP Server: %s", NTPServer.c_str());
            // ToDo: basic error testing
        }
        Settings::NTP::setServer(NTPServer);

        // request interval (in min)
        const String strNTPInterval = srv->arg("ntp-interval");
        log_v("NTPInterval: %s", strNTPInterval.c_str());
        auto interval = size_t(strNTPInterval.toInt());
        log_d("");
        switch(interval) {
            case 60:
            case 4 * 60:
            case 12 * 60:
            case 24 * 60:
            case 2 * 24 * 60:
            case 7 * 24 * 60:
                break;
            default:
                interval = 4 * 60;
        }
        log_d("");
        // Settings::NTP::setSyncInterval(interval);
        log_d("");
    } else {
        // get the time the user set
        const String localTime = srv->arg("set-time");
        log_v("Current time: %s", localTime.c_str());
        setSystemTime(localTime);
    }
    log_d("");
    saveRequested = true;
    log_d("");

    srv->send_P(200, PSTR("text/html"), PSTR("<script>parent.location.href = '/';</script>"));
}

void bindServerRequests() {
    _wm->server->on("/custom", handleRoute);
    _wm->server->on("/save-wc", handleValues);
}

} // namespace WordClockParams

} // namespace wc
