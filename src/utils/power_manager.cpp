/**
 * @file power_manager.cpp
 * @brief Power management implementation
 */

#include "power_manager.h"
#include "hardware/wiring.h"
#include <esp_sleep.h>
#include <driver/rtc_io.h>

void powerInit(void) {
    // 电源管理初始化
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
}

void powerDeepSleep(uint32_t sleepMinutes) {
    Serial.printf("[Power] Entering deep sleep for %d minutes\n", sleepMinutes);
    Serial.flush();
    
    // 配置定时器唤醒
    if (sleepMinutes > 0) {
        uint64_t sleepUs = (uint64_t)sleepMinutes * 60 * 1000000ULL;
        esp_sleep_enable_timer_wakeup(sleepUs);
    }
    
    // 进入深度休眠
    esp_deep_sleep_start();
}

void powerLightSleep(uint32_t sleepMs) {
    // 配置定时器唤醒
    if (sleepMs > 0) {
        uint64_t sleepUs = (uint64_t)sleepMs * 1000ULL;
        esp_sleep_enable_timer_wakeup(sleepUs);
    }
    
    // 进入轻度休眠
    esp_light_sleep_start();
}

WakeupReason powerGetWakeupReason(void) {
    esp_sleep_wakeup_cause_t reason = esp_sleep_get_wakeup_cause();
    
    switch (reason) {
        case ESP_SLEEP_WAKEUP_EXT0:
            return WAKEUP_REASON_EXT0;
        case ESP_SLEEP_WAKEUP_EXT1:
            return WAKEUP_REASON_EXT1;
        case ESP_SLEEP_WAKEUP_TIMER:
            return WAKEUP_REASON_TIMER;
        case ESP_SLEEP_WAKEUP_TOUCHPAD:
            return WAKEUP_REASON_TOUCH;
        case ESP_SLEEP_WAKEUP_ULP:
            return WAKEUP_REASON_ULP;
        default:
            // 检查是否为上电启动
            if (reason == ESP_SLEEP_WAKEUP_UNDEFINED) {
                return WAKEUP_REASON_POWER_ON;
            }
            return WAKEUP_REASON_UNKNOWN;
    }
}

void powerPrintWakeupReason(void) {
    WakeupReason reason = powerGetWakeupReason();
    
    Serial.println("===== Wakeup Reason =====");
    switch (reason) {
        case WAKEUP_REASON_POWER_ON:
            Serial.println("Power on reset");
            break;
        case WAKEUP_REASON_EXT0:
            Serial.println("External interrupt EXT0");
            break;
        case WAKEUP_REASON_EXT1:
            Serial.println("External interrupt EXT1");
            break;
        case WAKEUP_REASON_TIMER:
            Serial.println("Timer wakeup");
            break;
        case WAKEUP_REASON_TOUCH:
            Serial.println("Touchpad wakeup");
            break;
        case WAKEUP_REASON_ULP:
            Serial.println("ULP wakeup");
            break;
        default:
            Serial.println("Unknown");
            break;
    }
    Serial.println("=========================");
}

void powerEnableGpioWakeup(uint8_t gpioNum, uint8_t level) {
    // ESP32-C3 deep sleep GPIO wakeup needs esp_deep_sleep_enable_gpio_wakeup.
    gpio_pullup_en((gpio_num_t)gpioNum);
    gpio_pulldown_dis((gpio_num_t)gpioNum);

    uint64_t gpioMask = 1ULL << gpioNum;
    esp_deepsleep_gpio_wake_up_mode_t mode = level ?
        ESP_GPIO_WAKEUP_GPIO_HIGH :
        ESP_GPIO_WAKEUP_GPIO_LOW;
    esp_err_t err = esp_deep_sleep_enable_gpio_wakeup(gpioMask, mode);

    Serial.printf("[Power] GPIO wakeup GPIO%u level=%u: %s\n",
                  gpioNum, level, esp_err_to_name(err));
}

void powerEnableTimerWakeup(uint32_t minutes) {
    uint64_t sleepUs = (uint64_t)minutes * 60 * 1000000ULL;
    esp_sleep_enable_timer_wakeup(sleepUs);
}

void powerDisableAllWakeup(void) {
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
}
