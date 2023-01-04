#include <Arduino.h>
#include <time.h>

#include "c++23.h"
#include "config.h"
#include "esp-hal-log.h"

#include "Settings.h"
#include "WordClock.h"
#include "WordClockPage.h"
#include "genTimezone.h"

constexpr const char *menuhtml PROGMEM = "<form action='/custom' method='get'><button>Setup Clock</button></form><br/>";
constexpr std::array<std::pair<int, const char *>, 5> syncDefault PROGMEM
    = {{{60, "Hourly"}, {240, "Every 12 hour"}, {1140, "Daily"}, {2880, "Every 2 days"}, {10080, "Weekly"}}};

WordClockPage wordClockPage;

void WordClockPage::begin(WiFiManager *wm) {
    this->wm = wm;
    wm->setWebServerCallback(std::bind(&WordClockPage::bindServerRequests, this));
    std::vector<const char *> menu = {"wifi", "custom", "info", "param", "sep", "restart", "exit"};
    wm->setMenu(menu);
    wm->setCustomMenuHTML(menuhtml);
}

void WordClockPage::handleRoute() {
    log_d("HTTP] Handle route Custom");

    String TimeConfHTML;
    TimeConfHTML.reserve(28000);
    TimeConfHTML += HTTP_HEAD_START;
    TimeConfHTML.replace(FPSTR(T_v), "Word Clock setup");

    TimeConfHTML += HTTP_SCRIPT;
    TimeConfHTML += F("<script>;window.addEventListener('load', function() { var now = new Date(); "
                      "document.getElementById('set-time').value = now.toISOString().substring(0,16); });"
                      "</script>");
    TimeConfHTML += HTTP_STYLE;
    TimeConfHTML += F("<style>input[type='checkbox'][name='use-ntp-server']:not(:checked) ~.collapsable{display:none;}"
                      "input[type='checkbox'][name='use-ntp-server']:checked ~.collapsed{display:none;}</style>");
    TimeConfHTML += HTTP_HEAD_END;
    TimeConfHTML += F("<iframe name='dummyframe' id='dummyframe' style='display: none;'></iframe>"
                      "<form action='/save-wc' target='dummyframe' method='POST' novalidate>");
    const int brightness = BrightnessToIndex(settings.brightness);
    TimeConfHTML += F("<h1>WordClock Settings</h1>"
                      "<p>Brightness</p>"
                      "<input style='display: inline-block;' type='radio' id='choice1' name='brightness' value='0' ");
    TimeConfHTML += (brightness == 0) ? "checked>" : ">";
    TimeConfHTML += F("<label for='choice1'>Low</label><br/>"
                      "<input style='display: inline-block;' type='radio' id='choice2' name='brightness' value='1' ");
    TimeConfHTML += (brightness == 1) ? "checked>" : ">";
    TimeConfHTML += F("<label for='choice2'>Medium</label><br/>"
                      "<input style='display: inline-block;' type='radio' id='choice3' name='brightness' value='2' ");
    TimeConfHTML += (brightness == 2) ? "checked>" : ">";
    TimeConfHTML += F("<label for='choice3'>high</label><br/>");
#ifdef NIGHTMODE
    TimeConfHTML += F("<input style='display: inline-block;' type='radio' id='choice4' name='brightness' value='3' ");
    TimeConfHTML += (brightness == 3) ? "checked>" : ">";
    TimeConfHTML += F("<label for='choice4'>night</label>");
#endif
    TimeConfHTML += F("<br /><br /> "
                      "<label for='palette'>Color Palette</label>"
                      "<select name='palette' id='palette' class='button'>");
    uint8_t i = 0;
    for(const auto p : data::paletteNames) {
        TimeConfHTML += F("<option value='") + String(i);
        TimeConfHTML += (settings.palette == i) ? "' selected>" : "'>";
        TimeConfHTML += String(p) + F("</option>");
        i++;
    }
    TimeConfHTML += F("</select>"
                      "<h1>Time Settings</h1>"
                      "<label for='timezone'>Time Zone</label>"
                      "<select id='timezone' name='timezone'>");
    for(size_t i = 0; i < timezoneSize; i++) {
        bool selected = (settings.timezone == i);
        TimeConfHTML += F("<option value='") + String(i) + String(selected ? "' selected>" : "'>");
        TimeConfHTML += String(FPSTR(timezones[i][0])) + "</option>";
    }
    TimeConfHTML += F("</select><br><br>"
                      "<label for='use-wifi'>Enable portal on startup (wifi always on)</label>"
                      "<input value='1' type=checkbox name='use-wifi' id='use-wifi'");
    TimeConfHTML += String(settings.wifiEnable ? "checked>" : ">");
    TimeConfHTML += F("</select><br><br>"
                      "<label for='use-ntp-server'>Enable NTP Client</label> "
                      "<input value='1' type=checkbox name='use-ntp-server' id='use-ntp-server'");
    TimeConfHTML += String(settings.ntpEnabled ? "checked>" : ">");
    TimeConfHTML += F("<br/>"
                      "<div class='collapsed'>"
                      "<label for='set-time'>Set Time (UTC)"
                      "<input style=width:auto name='set-time' step='1' id='set-time' type='datetime-local'></div>"
                      "<div class='collapsable'>"
                      "<h2>NTP Client Setup</h2>"
                      "<br><label for='ntp-server'>Server:</label>"
                      "<input type='text' id='ntp-server' name='ntp-server' value='");
    TimeConfHTML += settings.ntpServer;
    TimeConfHTML += F("'><br>"
                      "<label for='ntp-interval'>Sync interval:</label>"
                      "<select id='ntp-interval' name='ntp-interval'>");
    for(const auto &[min, name] : syncDefault) {
        TimeConfHTML += F("<option value='") + String(min);
        TimeConfHTML += (settings.syncInterval == min) ? "' selected>" : "'>";
        TimeConfHTML += String(name) + "</option>";
    }
    TimeConfHTML += F("</select><br>"
                      "</div>");
    TimeConfHTML += F("<h2>Night Mode Setup</h2>"
                      "<label for='use-night-mode'>Enable automatic Night Mode</label> "
                      " <input value='1' type=checkbox name='use-night-mode' id='use-night-mode'");
    TimeConfHTML += String(settings.nmEnable ? "checked>" : ">");
    TimeConfHTML += F("<br><label for='nm-start'>Start Time:</label><input style=width:auto type='time' name='nm-start' id='nm-start' value='");
    TimeConfHTML += settings.nmStartTime.toString() + "'>";
    TimeConfHTML += F("<br><label for='nm-end'>End Time:</label><input style=width:auto type='time' name='nm-end' id='nm-end' value='");
    TimeConfHTML += settings.nmEndTime.toString() + "'>";

    TimeConfHTML += F("<br><br><button type=submit>Submit</button></form>");
    TimeConfHTML += HTTP_END;

    wm->server->send(200, "text/html", TimeConfHTML.c_str());
}

void WordClockPage::handleValues() {
    log_d("[HTTP] handle route Values");
    auto &srv = wm->server;

    // WordClock
    if(srv->hasArg("brightness")) {
        const String strBrightness = srv->arg("brightness");
        log_v("brightness: %s", strBrightness.c_str());
        int brightness = std::min(int(strBrightness.toInt()), std::to_underlying(Brightness::END_OF_LIST) - 1);
        settings.brightness = static_cast<Brightness>(brightness);
    }

    if(srv->hasArg("palette")) {
        const String strPalette = srv->arg("palette");
        log_v("palette: %s", strPalette.c_str());
        int palette = std::min(uint32_t(strPalette.toInt()), data::colorPalettes.size() - 1);
        settings.palette.currentPalette = palette;
    }

    // Timezones
    if(srv->hasArg("timezone")) {
        const String strTz = srv->arg("timezone");
        log_v("Timezone: %s", strTz.c_str());
        size_t tzId = strTz.toInt();
        if(tzId >= timezoneSize)
            tzId = TZ_Names::TZ_Etc_UTC;
        settings.timezone = tzId;
    }

    // WIFI
    const String useWiFi = srv->arg("use-wifi");
    log_v("useWiFi: %s", useWiFi.c_str());
    const bool wifiEnabled = useWiFi.toInt() == 1;
    settings.wifiEnable = wifiEnabled;

    // NTP
    const String useNtpServer = srv->arg("use-ntp-server");
    log_v("UseNtpServer: %s", useNtpServer.c_str());
    const bool NTPEnabled = useNtpServer.toInt() == 1;
    settings.ntpEnabled = NTPEnabled;

    if(NTPEnabled) {
        // get the rest of the args

        // NTP server
        String NTPServer = "pool.ntp.org";
        if(srv->hasArg("ntp-server")) {
            String NTPServer = srv->arg("ntp-server");
            log_v("NTP Server: %s", NTPServer.c_str());
        }
        settings.ntpServer = NTPServer;

        // request interval (in min)
        const String strNTPInterval = srv->arg("ntp-interval");
        log_v("NTPInterval: %s", strNTPInterval.c_str());
        auto interval = size_t(strNTPInterval.toInt());
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
        settings.syncInterval = interval;
    } else {
        // get the time the user set
        const String localTime = srv->arg("set-time");
        log_v("Current time: %s", localTime.c_str());
        struct tm tm = {0};
        strptime(localTime.c_str(), "%FT%T", &tm);
        time_t now = mktime(&tm) - _timezone;
        log_d("UTC time: %lld", now);
        wordClock.adjustInternalTime(now);
        wordClock.timeUpdate(true);
    }

    // night mode
    const String useNightMode = srv->arg("use-night-mode");
    log_v("useNightMode: %s", useNightMode.c_str());
    const bool nmEnable = useNightMode.toInt() == 1;
    settings.nmEnable = nmEnable;

    if(nmEnable) {
        // get the start time
        const String nmStart = srv->arg("nm-start");
        log_v("nmStart: %s", nmStart.c_str());
        if(!settings.nmStartTime.parseString(nmStart.c_str())) {
            log_v("Failed to parse start time");
            settings.nmStartTime = {22, 0};
        }

        // and the end time
        const String nmEnd = srv->arg("nm-end");
        log_v("nmEnd: %s", nmEnd.c_str());
        if(!settings.nmEndTime.parseString(nmEnd.c_str())) {
            log_v("Failed to parse start time");
            settings.nmEndTime = {8, 0};
        }
    }

    settings.requestAsyncSave();

    srv->send_P(200, PSTR("text/html"), PSTR("<script>parent.location.href = '/';</script>"));
}

void WordClockPage::bindServerRequests() {
    wm->server->on("/custom", std::bind(&WordClockPage::handleRoute, this));
    wm->server->on("/save-wc", std::bind(&WordClockPage::handleValues, this));
}