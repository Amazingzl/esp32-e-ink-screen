#include <Arduino.h>

#include "hardware/wiring.h"
#include "hardware/led.h"
#include "hardware/button.h"
#include "hardware/battery.h"

#include "display/display_manager.h"
#include "display/epaper_driver.h"
#include "display/layouts/weather_layout.h"

#include "network/wifi_manager.h"
#include "network/ntp_sync.h"
#include "network/weather_api.h"

#include "utils/preferences.h"
#include "utils/power_manager.h"

#define APP_NAME        "ESP32-C3-EPaper"
#define APP_VERSION     "1.0.0"

#define WIFI_CONNECT_TIMEOUT_MS     10000
#define NTP_SYNC_TIMEOUT_MS         10000
#define SLEEP_MINUTES               120
#define WEATHER_REFRESH_MS          (60UL * 60UL * 1000UL)
#define CONFIG_PORTAL_MINUTES       5
#define CONFIG_AP_NAME              "C3EPAPER"
#define CONFIG_AP_PASS              ""

static bool gWifiConnected = false;
static bool gTimeSynced = false;
static bool gWeatherReady = false;
static unsigned long gLastActivity = 0;
static unsigned long gLastWeatherRefreshMs = 0;
static int gLastDisplayedMinute = -1;
static int gLastDailySyncYday = -1;

static WeatherForecastData gWeatherData;
static WeatherLayout gWeatherLayout;

void setupHardware(void);
void setupDisplay(void);
bool setupNetwork(void);
void refreshData(void);
void runConfigPortal(void);
void drawHomeScreen(void);
void drawStatusScreen(const char* status);
void drawConfigScreen(void);
void updateTimePartial(void);
void handleScheduledTasks(void);
void onButtonClick(void);
void onButtonDoubleClick(void);
void onButtonLongPress(void);
void enterDeepSleep(void);

void setup() {
    Serial.begin(115200);
    delay(100);

    Serial.println("\n\n=================================");
    Serial.printf("  %s v%s\n", APP_NAME, APP_VERSION);
    Serial.println("=================================\n");
    gLastActivity = millis();

    powerPrintWakeupReason();

    setupHardware();

    batteryPrintInfo();
    if (batteryGetStatus() == BATTERY_CRITICAL) {
        Serial.println("[Main] Battery critical, going to sleep");
        enterDeepSleep();
    }

#ifdef DISABLE_EPAPER
    Serial.println("[Display] Disabled by DISABLE_EPAPER");
#else
    setupDisplay();
    gWeatherLayout.init();
#endif

    if (!weatherInit()) {
        Serial.println("[Main] Weather API not configured, will use placeholder");
        drawStatusScreen("Set API Key in weather_api.h");
    } else if (!wifiHasCredentials()) {
        Serial.println("[Main] No saved WiFi credentials, starting config portal");
        runConfigPortal();
        drawHomeScreen();
    } else {
        refreshData();
        if (!gWifiConnected) {
            runConfigPortal();
        }
        drawHomeScreen();
    }

    wifiPowerOff();
    gLastWeatherRefreshMs = millis();
    struct tm bootTime = ntpGetTime();
    gLastDisplayedMinute = bootTime.tm_min;
    gLastDailySyncYday = bootTime.tm_yday;

    Serial.println("[Main] Setup complete");
}

void loop() {
    ledLoop();

#ifdef DISABLE_EPAPER
    delay(100);
    return;
#endif

    buttonTick();
    handleScheduledTasks();
    delay(50);
}

void setupHardware(void) {
    Serial.println("[Hardware] Initializing...");

    ledInit();
    ledFastBlink();

#ifdef DISABLE_EPAPER
    Serial.println("[Button] Disabled in DISABLE_EPAPER mode");
#else
    buttonInit();
    buttonAttachClick(onButtonClick);
    buttonAttachDoubleClick(onButtonDoubleClick);
    buttonAttachLongPress(onButtonLongPress);
#endif

    batteryInit();
    prefsInit();
    powerInit();

    Serial.println("[Hardware] Initialized");
}

void setupDisplay(void) {
#ifdef DISABLE_EPAPER
    Serial.println("[Display] Disabled, skip init");
    return;
#endif

    Serial.println("[Display] Initializing...");

    displayManager.init();

    Serial.printf("[Display] Resolution: %dx%d\n",
                  displayManager.width(), displayManager.height());
    Serial.printf("[Display] Partial update: %s\n",
                  epaperHasPartialUpdate() ? "Yes" : "No");
}

bool setupNetwork(void) {
    Serial.println("[Network] Initializing...");

    wifiInit();

    if (wifiConnect(WIFI_CONNECT_TIMEOUT_MS)) {
        ledOn();
        gWifiConnected = true;
        ntpInit();
        return true;
    } else {
        ledSlowBlink();
        gWifiConnected = false;
        return false;
    }
}

void refreshData(void) {
    gWifiConnected = false;
    gTimeSynced = false;
    gWeatherReady = false;

    if (setupNetwork()) {
        gTimeSynced = ntpSync(NTP_SYNC_TIMEOUT_MS);

        if (weatherInit()) {
            Serial.println("[Weather] Fetching hourly forecast...");
            gWeatherReady = weatherFetchHourly(&gWeatherData, WEATHER_DISPLAY_HOURS);
            if (gWeatherReady) {
                weatherPrintData(&gWeatherData);
            } else {
                Serial.println("[Weather] Fetch failed");
            }
        }
    }
}

void runConfigPortal(void) {
    gLastActivity = millis();
    ledTripleBlink();

    wifiInit();
    if (wifiStartConfigMode(CONFIG_AP_NAME, CONFIG_AP_PASS, CONFIG_PORTAL_MINUTES, drawConfigScreen)) {
        ntpInit();
        gWifiConnected = true;
        gTimeSynced = ntpSync(NTP_SYNC_TIMEOUT_MS);

        if (weatherInit()) {
            gWeatherReady = weatherFetchHourly(&gWeatherData, WEATHER_DISPLAY_HOURS);
        }
    } else {
        gWifiConnected = false;
        gTimeSynced = false;
        gWeatherReady = false;
    }
    gLastActivity = millis();
}

void drawHomeScreen(void) {
    Serial.println("[UI] Drawing weather screen");

#ifdef DISABLE_EPAPER
    Serial.println("[UI] E-paper disabled, skip drawing");
    return;
#endif

    Serial.println("[UI] Reading time");
    struct tm timeInfo = ntpGetTime();
    Serial.printf("[UI] Time: %04d-%02d-%02d wday=%d\n",
                  timeInfo.tm_year + 1900, timeInfo.tm_mon + 1,
                  timeInfo.tm_mday, timeInfo.tm_wday);

    Serial.println("[UI] Reading battery voltage");
    uint16_t voltage = batteryReadVoltage();
    Serial.printf("[UI] Voltage: %u\n", voltage);

    Serial.println("[UI] Reading battery percent");
    uint8_t battery = batteryGetPercentage();
    Serial.printf("[UI] Battery: %u\n", battery);

    Serial.println("[UI] Using initialized display");

    if (gWeatherReady) {
        Serial.println("[UI] Calling weather layout");
        gWeatherLayout.draw(&gWeatherData, &timeInfo, battery, voltage, SLEEP_MINUTES);
    } else {
        Serial.println("[UI] Calling no-data layout");
        gWeatherLayout.drawNoData(gTimeSynced ?
            "Weather fetch failed. Check API key." :
            "Time not synced. Configure WiFi.");
    }
}

void updateTimePartial(void) {
#ifdef DISABLE_EPAPER
    return;
#endif

    struct tm timeInfo = ntpGetTime();
    if (timeInfo.tm_year < 100) {
        return;
    }

    if (timeInfo.tm_min == gLastDisplayedMinute) {
        return;
    }

    gLastDisplayedMinute = timeInfo.tm_min;
    gWeatherLayout.drawTimePartial(&timeInfo);
}

void handleScheduledTasks(void) {
    struct tm timeInfo = ntpGetTime();
    if (timeInfo.tm_year < 100) {
        return;
    }

    updateTimePartial();

    bool needsWeatherRefresh = millis() - gLastWeatherRefreshMs >= WEATHER_REFRESH_MS;
    bool needsDailyRefresh = timeInfo.tm_hour == 0 && timeInfo.tm_min == 0 &&
                             timeInfo.tm_yday != gLastDailySyncYday;

    if (!needsWeatherRefresh && !needsDailyRefresh) {
        return;
    }

    Serial.printf("[Schedule] Refresh, hourly=%s daily=%s\n",
                  needsWeatherRefresh ? "yes" : "no",
                  needsDailyRefresh ? "yes" : "no");

    if (setupNetwork()) {
        if (needsDailyRefresh) {
            gTimeSynced = ntpSync(NTP_SYNC_TIMEOUT_MS);
            timeInfo = ntpGetTime();
            gLastDailySyncYday = timeInfo.tm_yday;
        }

        if (weatherInit()) {
            gWeatherReady = weatherFetchHourly(&gWeatherData, WEATHER_DISPLAY_HOURS);
            if (gWeatherReady) {
                weatherPrintData(&gWeatherData);
            }
        }

        wifiPowerOff();
        gLastWeatherRefreshMs = millis();
        drawHomeScreen();
    } else {
        wifiPowerOff();
        gLastWeatherRefreshMs = millis();
        if (needsDailyRefresh) {
            gLastDailySyncYday = timeInfo.tm_yday;
        }
    }
}

void drawStatusScreen(const char* status) {
#ifdef DISABLE_EPAPER
    Serial.print("[UI] Status screen skipped: ");
    Serial.println(status);
    return;
#endif

    epaperReinit(false);
    displayManager.setFullWindow();
    displayManager.firstPage();

    do {
        displayManager.fillScreen(COLOR_WHITE);
        displayManager.drawRect(10, 10, displayManager.width() - 20, displayManager.height() - 20, COLOR_BLACK);
        displayManager.setTextSize(2);
        displayManager.setCursor(30, 70);
        displayManager.setTextColor(COLOR_BLACK);
        displayManager.print("Weather Display");
        displayManager.setTextSize(1);
        displayManager.setCursor(30, 130);
        displayManager.print(status);
    } while (displayManager.nextPage());
}

void drawConfigScreen(void) {
#ifdef DISABLE_EPAPER
    Serial.println("[UI] Config screen skipped");
    return;
#endif

    epaperReinit(false);
    displayManager.setFullWindow();
    displayManager.firstPage();

    do {
        displayManager.fillScreen(COLOR_WHITE);
        displayManager.drawRect(10, 10, displayManager.width() - 20, displayManager.height() - 20, COLOR_BLACK);
        displayManager.setTextColor(COLOR_BLACK);
        displayManager.setTextSize(2);
        displayManager.setCursor(28, 52);
        displayManager.print("WiFi Config Mode");

        displayManager.setTextSize(1);
        displayManager.setCursor(30, 108);
        displayManager.print("AP: ");
        displayManager.print(CONFIG_AP_NAME);
        displayManager.setCursor(30, 132);
        displayManager.print("Pass: ");
        displayManager.print(CONFIG_AP_PASS);
        displayManager.setCursor(30, 156);
        displayManager.print("Connect phone to AP,");
        displayManager.setCursor(30, 172);
        displayManager.print("then open 192.168.4.1");
        displayManager.setCursor(30, 190);
        displayManager.print("Timeout: ");
        displayManager.print(CONFIG_PORTAL_MINUTES);
        displayManager.print(" min");
        displayManager.setCursor(30, 210);
        displayManager.print("Click=Refresh  Long=Clear WiFi");
    } while (displayManager.nextPage());
}

void onButtonClick(void) {
    Serial.println("[Button] Click");
    gLastActivity = millis();

    drawStatusScreen("Refreshing...");
    refreshData();
    drawHomeScreen();
    wifiPowerOff();
    gLastWeatherRefreshMs = millis();
    gLastDisplayedMinute = ntpGetMinute();
}

void onButtonDoubleClick(void) {
    Serial.println("[Button] Double click");
    gLastActivity = millis();

    runConfigPortal();
    drawHomeScreen();
    wifiPowerOff();
    gLastWeatherRefreshMs = millis();
    gLastDisplayedMinute = ntpGetMinute();
    gLastActivity = millis();
}

void onButtonLongPress(void) {
    Serial.println("[Button] Long press - Clear WiFi");
    gLastActivity = millis();

    drawStatusScreen("Clearing WiFi...");
    wifiClearCredentials();
    delay(1000);

    drawStatusScreen("WiFi cleared.\nDouble click to config.");
    delay(2000);

    drawStatusScreen("Going to sleep...");
    delay(500);
    enterDeepSleep();
}

void enterDeepSleep(void) {
    Serial.println("[Power] Preparing for deep sleep...");

#ifdef DISABLE_EPAPER
    Serial.println("[Power] Deep sleep skipped in DISABLE_EPAPER mode");
    return;
#endif

    wifiPowerOff();

    displayManager.hibernate();
    ledOff();

    powerDisableAllWakeup();
    powerEnableGpioWakeup(KEY_M, 0);
    powerEnableTimerWakeup(SLEEP_MINUTES);

    powerDeepSleep(0);
}
