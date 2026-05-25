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

// ============================================
// Function Prototypes
// ============================================
void setupHardware(void);
void setupDisplay(void);
bool setupNetwork(void);
void drawHelloWorld(void);
void drawStatusScreen(const char* status);
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
    
    // 显示启动画面
    drawStatusScreen("Starting...");
    
    // 尝试连接网络
    if (setupNetwork()) {
        // 同步时间
        ntpSync(NTP_SYNC_TIMEOUT_MS);
        drawStatusScreen("Time Synced");
        delay(500);
    }
    
    // 显示主界面
    drawHelloWorld();
    
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
    
    // 示例：10秒后进入休眠
    static unsigned long lastActivity = millis();
    if (millis() - lastActivity > 10000) {
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
        
        // 初始化 NTP
        ntpInit();
        return true;
    } else {
        ledSlowBlink();  // 慢闪表示连接失败
        return false;
    }
}

// ============================================
// UI Functions
// ============================================
void drawHelloWorld(void) {
    Serial.println("[UI] Drawing Hello World");
    
    // 全屏刷新示例
    displayManager.setFullWindow();
    displayManager.firstPage();
    
    do {
        // 清屏
        displayManager.fillScreen(COLOR_WHITE);
        
        // 绘制边框
        displayManager.drawRect(10, 10, 
                                displayManager.width() - 20, 
                                displayManager.height() - 20, 
                                COLOR_BLACK);
        
        // 绘制标题
        displayManager.setCursor(50, 50);
        displayManager.setTextColor(COLOR_BLACK);
        displayManager.print("ESP32 C3 E-Paper");
        
        // 绘制版本
        displayManager.setCursor(50, 80);
        displayManager.print("Version: ");
        displayManager.print(APP_VERSION);
        
        // 绘制时间（如果已同步）
        if (ntpIsSynced()) {
            displayManager.setCursor(50, 120);
            displayManager.print("Time: ");
            displayManager.print(ntpFormatTime("%H:%M:%S").c_str());
            
            displayManager.setCursor(50, 150);
            displayManager.print("Date: ");
            displayManager.print(ntpFormatTime("%Y-%m-%d").c_str());
        }
        
        // 绘制电池信息
        displayManager.setCursor(50, 200);
        displayManager.print("Battery: ");
        displayManager.print(batteryGetPercentage());
        displayManager.print("%");
        
        // 绘制提示
        displayManager.setCursor(50, 250);
        displayManager.setTextColor(COLOR_RED);
        displayManager.print("Click: Refresh | Double: Config | Long: Sleep");
        
    } while (displayManager.nextPage());
    
    // 全屏刷新
    displayManager.fullRefresh();
}

void drawStatusScreen(const char* status) {
    // 局部刷新示例 - 只刷新状态区域
    int16_t statusY = displayManager.height() - 40;
    int16_t statusH = 30;
    
    displayManager.setPartialWindow(0, statusY, displayManager.width(), statusH);
    displayManager.firstPage();
    
    do {
        displayManager.fillRect(0, statusY, displayManager.width(), statusH, COLOR_WHITE);
        displayManager.setCursor(20, statusY + 20);
        displayManager.setTextColor(COLOR_BLACK);
        displayManager.print(status);
    } while (displayManager.nextPage());
    
    // 局部刷新
    displayManager.partialRefresh();
}

// ============================================
// Button Handlers
// ============================================
void onButtonClick(void) {
    Serial.println("[Button] Click");
    
    // 刷新屏幕
    drawHelloWorld();
}

void onButtonDoubleClick(void) {
    Serial.println("[Button] Double click");
    
    // 进入配置模式（示例）
    drawStatusScreen("Config Mode...");
    ledTripleBlink();
}

void onButtonLongPress(void) {
    Serial.println("[Button] Long press");
    
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
