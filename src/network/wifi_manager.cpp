/**
 * @file wifi_manager.cpp
 * @brief WiFi manager implementation
 */

#include "wifi_manager.h"
#include "utils/preferences.h"
#include <WiFi.h>

static WifiStatus _status = WIFI_DISCONNECTED;

void wifiInit(void) {
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(false);  // 我们手动控制重连
    _status = WIFI_DISCONNECTED;
}

bool wifiConnect(uint32_t timeoutMs) {
    char ssid[33] = {0};
    char password[65] = {0};
    
    // 读取保存的凭据
    if (prefsGetString(PREF_WIFI_SSID, ssid, sizeof(ssid)) == 0) {
        Serial.println("[WiFi] No saved credentials");
        _status = WIFI_CONNECTION_FAILED;
        return false;
    }
    prefsGetString(PREF_WIFI_PASS, password, sizeof(password));
    
    Serial.printf("[WiFi] Connecting to %s...\n", ssid);
    _status = WIFI_CONNECTING;
    
    WiFi.begin(ssid, password);
    
    // 等待连接
    uint32_t startTime = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - startTime >= timeoutMs) {
            Serial.println("[WiFi] Connection timeout");
            _status = WIFI_CONNECTION_FAILED;
            return false;
        }
        delay(100);
    }
    
    Serial.printf("[WiFi] Connected, IP: %s\n", WiFi.localIP().toString().c_str());
    _status = WIFI_CONNECTED;
    return true;
}

void wifiDisconnect(void) {
    WiFi.disconnect();
    _status = WIFI_DISCONNECTED;
}

WifiStatus wifiGetStatus(void) {
    // 更新状态
    if (_status == WIFI_CONNECTING) {
        if (WiFi.status() == WL_CONNECTED) {
            _status = WIFI_CONNECTED;
        }
    } else if (_status == WIFI_CONNECTED) {
        if (WiFi.status() != WL_CONNECTED) {
            _status = WIFI_DISCONNECTED;
        }
    }
    return _status;
}

bool wifiIsConnected(void) {
    return WiFi.status() == WL_CONNECTED;
}

int8_t wifiGetRSSI(void) {
    if (!wifiIsConnected()) return 0;
    return WiFi.RSSI();
}

String wifiGetLocalIP(void) {
    if (!wifiIsConnected()) return String("");
    return WiFi.localIP().toString();
}

bool wifiStartConfigMode(const char* apName, const char* apPass, uint8_t timeoutMinutes) {
    Serial.printf("[WiFi] Starting AP mode: %s\n", apName);
    
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apName, apPass);
    
    Serial.printf("[WiFi] AP IP: %s\n", WiFi.softAPIP().toString().c_str());
    
    // 等待配置（简化实现，实际可以使用 WiFiManager 库）
    if (timeoutMinutes > 0) {
        delay(timeoutMinutes * 60 * 1000);
    }
    
    return false;  // 简化实现，实际应返回配置结果
}

void wifiSaveCredentials(const char* ssid, const char* password) {
    prefsPutString(PREF_WIFI_SSID, ssid);
    prefsPutString(PREF_WIFI_PASS, password);
}

void wifiClearCredentials(void) {
    prefsRemove(PREF_WIFI_SSID);
    prefsRemove(PREF_WIFI_PASS);
}

void wifiPowerOff(void) {
    WiFi.mode(WIFI_OFF);
    _status = WIFI_DISCONNECTED;
    Serial.println("[WiFi] Powered off");
}
