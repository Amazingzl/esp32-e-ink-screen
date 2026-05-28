/**
 * @file main.cpp
 * @brief ESP32 C3 E-Paper Display Main Application
 * 
 * ESP32 C3 墨水屏主程序
 * 功能：
 * - 墨水屏显示（支持局部刷新）
 * - WiFi 连接
 * - NTP 时间同步
 * - 深度休眠省电
 * - 按键交互
 */

#include <Arduino.h>

// 硬件抽象层
#include "hardware/wiring.h"
#include "hardware/led.h"
#include "hardware/button.h"
#include "hardware/battery.h"

// 显示层
#include "display/display_manager.h"
#include "display/epaper_driver.h"

// 网络层
#include "network/wifi_manager.h"
#include "network/ntp_sync.h"

// 工具层
#include "utils/preferences.h"
#include "utils/power_manager.h"

// ============================================
// Version Info
// ============================================
#define APP_NAME        "ESP32-C3-EPaper"
#define APP_VERSION     "1.0.0"

// ============================================
// Configuration
// ============================================
#define WIFI_CONNECT_TIMEOUT_MS     10000   // WiFi连接超时
#define NTP_SYNC_TIMEOUT_MS         5000    // NTP同步超时
#define SLEEP_MINUTES               30      // 休眠时长（分钟）
#define SLEEP_AFTER_MS              15000   // 刷新后保持唤醒时长
#define CONFIG_PORTAL_MINUTES       5       // 配网门户超时
#define CONFIG_AP_NAME              "C3EPAPER"
#define CONFIG_AP_PASS              ""

static bool gWifiConnected = false;
static bool gTimeSynced = false;
static unsigned long gLastActivity = 0;

// ============================================
// Function Prototypes
// ============================================
void setupHardware(void);
void setupDisplay(void);
bool setupNetwork(void);
void refreshData(void);
void runConfigPortal(void);
void drawHomeScreen(void);
void drawStatusScreen(const char* status);
void drawConfigScreen(void);
void onButtonClick(void);
void onButtonDoubleClick(void);
void onButtonLongPress(void);
void enterDeepSleep(void);

// ============================================
// Setup
// ============================================
void setup() {
    // 初始化串口
    Serial.begin(115200);
    delay(100);
    
    Serial.println("\n\n=================================");
    Serial.printf("  %s v%s\n", APP_NAME, APP_VERSION);
    Serial.println("=================================\n");
    gLastActivity = millis();
    
    // 打印唤醒原因
    powerPrintWakeupReason();
    
    // 初始化硬件
    setupHardware();
    
    // 检查电池
    batteryPrintInfo();
    if (batteryGetStatus() == BATTERY_CRITICAL) {
        Serial.println("[Main] Battery critical, going to sleep");
        enterDeepSleep();
    }
    
    // 初始化显示
    setupDisplay();
    
    drawStatusScreen("Starting...");
    refreshData();
    if (!gWifiConnected) {
        runConfigPortal();
    }
    drawHomeScreen();
    
    // 关闭 WiFi 省电
    wifiPowerOff();
    
    Serial.println("[Main] Setup complete");
}

// ============================================
// Loop
// ============================================
void loop() {
    // 处理 LED 状态
    ledLoop();
    
    // 处理按键
    buttonTick();
    
    // 这里可以添加其他需要持续运行的任务
    // 但对于墨水屏应用，通常进入休眠更合适
    
    if (millis() - gLastActivity > SLEEP_AFTER_MS) {
        Serial.println("[Main] Entering sleep...");
        delay(100);
        enterDeepSleep();
    }
}

// ============================================
// Hardware Setup
// ============================================
void setupHardware(void) {
    Serial.println("[Hardware] Initializing...");
    
    // LED
    ledInit();
    ledFastBlink();  // 快闪表示启动中
    
    // 按键
    buttonInit();
    buttonAttachClick(onButtonClick);
    buttonAttachDoubleClick(onButtonDoubleClick);
    buttonAttachLongPress(onButtonLongPress);
    
    // 电池检测
    batteryInit();
    
    // 配置存储
    prefsInit();
    
    // 电源管理
    powerInit();
    
    Serial.println("[Hardware] Initialized");
}

// ============================================
// Display Setup
// ============================================
void setupDisplay(void) {
    Serial.println("[Display] Initializing...");
    
    displayManager.init();
    
    Serial.printf("[Display] Resolution: %dx%d\n", 
                  displayManager.width(), displayManager.height());
    Serial.printf("[Display] Partial update: %s\n", 
                  epaperHasPartialUpdate() ? "Yes" : "No");
}

// ============================================
// Network Setup
// ============================================
bool setupNetwork(void) {
    Serial.println("[Network] Initializing...");
    
    wifiInit();
    
    // 尝试连接 WiFi
    if (wifiConnect(WIFI_CONNECT_TIMEOUT_MS)) {
        ledOn();  // 常亮表示连接成功
        gWifiConnected = true;
        
        // 初始化 NTP
        ntpInit();
        return true;
    } else {
        ledSlowBlink();  // 慢闪表示连接失败
        gWifiConnected = false;
        return false;
    }
}

// ============================================
// UI Functions
// ============================================
void refreshData(void) {
    gWifiConnected = false;
    gTimeSynced = false;

    if (setupNetwork()) {
        gTimeSynced = ntpSync(NTP_SYNC_TIMEOUT_MS);
    }
}

void runConfigPortal(void) {
    gLastActivity = millis();
    ledTripleBlink();
    drawConfigScreen();

    wifiInit();
    if (wifiStartConfigMode(CONFIG_AP_NAME, CONFIG_AP_PASS, CONFIG_PORTAL_MINUTES)) {
        ntpInit();
        gWifiConnected = true;
        gTimeSynced = ntpSync(NTP_SYNC_TIMEOUT_MS);
    } else {
        gWifiConnected = false;
        gTimeSynced = false;
    }
    gLastActivity = millis();
}

void drawHomeScreen(void) {
    Serial.println("[UI] Drawing home screen");

    const char* weekdays[] = {
        "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"
    };

    struct tm timeInfo = ntpGetTime();
    uint16_t voltage = batteryReadVoltage();
    uint8_t battery = batteryGetPercentage();
    BatteryStatus batteryStatus = batteryGetStatus();

    epaperReinit(false);
    displayManager.setFullWindow();
    displayManager.firstPage();
    
    do {
        displayManager.fillScreen(COLOR_WHITE);

        displayManager.drawRect(6, 6, displayManager.width() - 12, displayManager.height() - 12, COLOR_BLACK);
        displayManager.fillRect(6, 6, displayManager.width() - 12, 34, COLOR_BLACK);
        displayManager.setTextSize(2);
        displayManager.setCursor(18, 16);
        displayManager.setTextColor(COLOR_WHITE);
        displayManager.print(APP_NAME);

        displayManager.setTextSize(1);
        displayManager.setCursor(304, 20);
        displayManager.print(APP_VERSION);

        displayManager.setTextSize(3);
        displayManager.setCursor(28, 78);
        displayManager.setTextColor(COLOR_BLACK);
        if (gTimeSynced) {
            char dateLine[24];
            snprintf(dateLine, sizeof(dateLine), "%04d-%02d-%02d",
                     timeInfo.tm_year + 1900, timeInfo.tm_mon + 1, timeInfo.tm_mday);
            displayManager.print(dateLine);
        } else {
            displayManager.print("Time not set");
        }

        displayManager.setTextSize(2);
        displayManager.setCursor(30, 122);
        if (gTimeSynced) {
            char timeLine[20];
            snprintf(timeLine, sizeof(timeLine), "%02d:%02d  %s",
                     timeInfo.tm_hour, timeInfo.tm_min, weekdays[timeInfo.tm_wday]);
            displayManager.print(timeLine);
        } else if (gWifiConnected) {
            displayManager.print("NTP sync failed");
        } else {
            displayManager.print("Double click to config WiFi");
        }

        displayManager.drawLine(18, 156, displayManager.width() - 18, 156, COLOR_BLACK);

        displayManager.setTextSize(1);
        displayManager.setCursor(28, 184);
        displayManager.print("WiFi: ");
        displayManager.print(gWifiConnected ? "Connected" : "Offline");
        if (gWifiConnected) {
            displayManager.print("  RSSI ");
            displayManager.print((int)wifiGetRSSI());
            displayManager.print(" dBm");
        }

        displayManager.setCursor(28, 206);
        displayManager.print("Battery: ");
        displayManager.print(battery);
        displayManager.print("%");
        displayManager.print("  ");
        displayManager.print((int)voltage);
        displayManager.print(" mV");
        if (batteryStatus == BATTERY_LOW) {
            displayManager.setTextColor(COLOR_RED);
            displayManager.print("  LOW");
            displayManager.setTextColor(COLOR_BLACK);
        }

        displayManager.setCursor(28, 228);
        displayManager.print("Next wake: ");
        displayManager.print(SLEEP_MINUTES);
        displayManager.print(" min");

        displayManager.fillRect(6, displayManager.height() - 34, displayManager.width() - 12, 28, COLOR_RED);
        displayManager.setTextColor(COLOR_WHITE);
        displayManager.setCursor(18, displayManager.height() - 16);
        displayManager.print("Click refresh | Double config | Long sleep");
    } while (displayManager.nextPage());
}

void drawStatusScreen(const char* status) {
    epaperReinit(false);
    displayManager.setFullWindow();
    displayManager.firstPage();
    
    do {
        displayManager.fillScreen(COLOR_WHITE);
        displayManager.drawRect(10, 10, displayManager.width() - 20, displayManager.height() - 20, COLOR_BLACK);
        displayManager.setTextSize(2);
        displayManager.setCursor(30, 70);
        displayManager.setTextColor(COLOR_BLACK);
        displayManager.print(APP_NAME);
        displayManager.setTextSize(2);
        displayManager.setCursor(30, 130);
        displayManager.print(status);
    } while (displayManager.nextPage());
}

void drawConfigScreen(void) {
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
        displayManager.setCursor(30, 170);
        displayManager.print("Open portal and save WiFi.");
        displayManager.setCursor(30, 202);
        displayManager.print("Timeout: ");
        displayManager.print(CONFIG_PORTAL_MINUTES);
        displayManager.print(" minutes");
    } while (displayManager.nextPage());
}

// ============================================
// Button Handlers
// ============================================
void onButtonClick(void) {
    Serial.println("[Button] Click");
    gLastActivity = millis();
    
    drawStatusScreen("Refreshing...");
    refreshData();
    drawHomeScreen();
    wifiPowerOff();
}

void onButtonDoubleClick(void) {
    Serial.println("[Button] Double click");
    gLastActivity = millis();

    runConfigPortal();
    drawHomeScreen();
    wifiPowerOff();
    gLastActivity = millis();
}

void onButtonLongPress(void) {
    Serial.println("[Button] Long press");
    gLastActivity = millis();
    
    // 立即进入休眠
    drawStatusScreen("Going to sleep...");
    delay(500);
    enterDeepSleep();
}

// ============================================
// Power Management
// ============================================
void enterDeepSleep(void) {
    Serial.println("[Power] Preparing for deep sleep...");
    
    // 关闭显示
    displayManager.hibernate();
    
    // LED 熄灭
    ledOff();
    
    // 配置唤醒源
    powerDisableAllWakeup();
    
    // 配置按键唤醒
    powerEnableGpioWakeup(KEY_M, 0);  // 低电平触发
    
    // 配置定时器唤醒
    powerEnableTimerWakeup(SLEEP_MINUTES);
    
    // 进入深度休眠
    powerDeepSleep(0);  // 0 表示使用已配置的定时器
}
