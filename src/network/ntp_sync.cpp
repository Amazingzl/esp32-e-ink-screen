/**
 * @file ntp_sync.cpp
 * @brief NTP time synchronization implementation
 */

#include "ntp_sync.h"
#include <WiFi.h>

static bool _synced = false;

void ntpInit(void) {
    // 配置 NTP 服务器（先配置服务器）
    configTime(0, 0, NTP_SERVER_1, NTP_SERVER_2, NTP_SERVER_3);
    
    // 配置时区（必须在 configTime 之后设置）
    setenv("TZ", DEFAULT_TIMEZONE, 1);
    tzset();
}

bool ntpSync(uint32_t timeoutMs) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[NTP] WiFi not connected");
        return false;
    }
    
    Serial.println("[NTP] Synchronizing time...");
    
    // 获取当前时间，带重试
    struct tm timeinfo;
    uint32_t retryDelay = 500;
    uint32_t elapsed = 0;
    
    while (elapsed < timeoutMs) {
        if (getLocalTime(&timeinfo, retryDelay / 1000)) {
            _synced = true;
            Serial.printf("[NTP] Time synced: %04d-%02d-%02d %02d:%02d:%02d\n",
                          timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                          timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
            return true;
        }
        
        elapsed += retryDelay;
        if (elapsed < timeoutMs) {
            Serial.printf("[NTP] Retry... (%d/%d ms)\n", elapsed, timeoutMs);
            delay(retryDelay);
        }
    }
    
    Serial.println("[NTP] Failed to get time after retries");
    _synced = false;
    return false;
}

bool ntpIsSynced(void) {
    return _synced;
}

struct tm ntpGetTime(void) {
    struct tm timeinfo;
    memset(&timeinfo, 0, sizeof(timeinfo));
    if (!getLocalTime(&timeinfo)) {
        // 返回 1970-01-01 如果未同步
        timeinfo.tm_year = 70;
        timeinfo.tm_mon = 0;
        timeinfo.tm_mday = 1;
        timeinfo.tm_hour = 0;
        timeinfo.tm_min = 0;
        timeinfo.tm_sec = 0;
        timeinfo.tm_wday = 4;
    }
    return timeinfo;
}

time_t ntpGetTimestamp(void) {
    return time(NULL);
}

String ntpFormatTime(const char* format) {
    struct tm timeinfo = ntpGetTime();
    char buffer[64];
    strftime(buffer, sizeof(buffer), format, &timeinfo);
    return String(buffer);
}

int ntpGetYear(void) {
    struct tm timeinfo = ntpGetTime();
    return timeinfo.tm_year + 1900;
}

int ntpGetMonth(void) {
    struct tm timeinfo = ntpGetTime();
    return timeinfo.tm_mon + 1;
}

int ntpGetDay(void) {
    struct tm timeinfo = ntpGetTime();
    return timeinfo.tm_mday;
}

int ntpGetHour(void) {
    struct tm timeinfo = ntpGetTime();
    return timeinfo.tm_hour;
}

int ntpGetMinute(void) {
    struct tm timeinfo = ntpGetTime();
    return timeinfo.tm_min;
}

int ntpGetSecond(void) {
    struct tm timeinfo = ntpGetTime();
    return timeinfo.tm_sec;
}

int ntpGetWeekday(void) {
    struct tm timeinfo = ntpGetTime();
    return timeinfo.tm_wday;
}
