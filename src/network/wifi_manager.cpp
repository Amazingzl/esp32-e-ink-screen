/**
 * @file wifi_manager.cpp
 * @brief WiFi manager implementation
 */

#include "wifi_manager.h"
#include "utils/preferences.h"
#include <WiFi.h>
#include <WiFiManager.h>
#include <esp_wifi.h>

static WifiStatus _status = WIFI_DISCONNECTED;
static bool _wifiEventRegistered = false;
static volatile uint8_t _lastDisconnectReason = 0;
static bool _suppressDisconnectLog = false;

static bool wifiIsAuthFailure(uint8_t reason) {
    return reason == WIFI_REASON_AUTH_EXPIRE ||
           reason == WIFI_REASON_AUTH_FAIL ||
           reason == WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT ||
           reason == WIFI_REASON_HANDSHAKE_TIMEOUT;
}

static void wifiApplyStaSettings(void) {
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.setTxPower(WIFI_POWER_19_5dBm);
    esp_wifi_set_ps(WIFI_PS_NONE);
    esp_wifi_set_bandwidth(WIFI_IF_STA, WIFI_BW_HT20);
    esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N);
}

static void wifiRegisterEventLogger(void) {
    if (_wifiEventRegistered) {
        return;
    }

    WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
        uint8_t reason = info.wifi_sta_disconnected.reason;
        _lastDisconnectReason = reason;
        if (_suppressDisconnectLog) {
            return;
        }
        Serial.printf("[WiFi] Disconnected, reason=%u (%s)\n",
                      reason,
                      WiFi.disconnectReasonName((wifi_err_reason_t)reason));
    }, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);

    _wifiEventRegistered = true;
}

static bool wifiReadStaCredentials(char* ssid, size_t ssidLen, char* password, size_t passwordLen) {
    wifi_config_t config;
    memset(&config, 0, sizeof(config));

    if (esp_wifi_get_config(WIFI_IF_STA, &config) != ESP_OK) {
        return false;
    }

    strlcpy(ssid, reinterpret_cast<const char*>(config.sta.ssid), ssidLen);
    strlcpy(password, reinterpret_cast<const char*>(config.sta.password), passwordLen);
    return ssid[0] != '\0';
}

void wifiInit(void) {
    wifiRegisterEventLogger();
    wifiApplyStaSettings();
    WiFi.setAutoReconnect(true);  // 启用自动重连
    WiFi.persistent(false);       // 使用项目自己的NVS凭据，避免ESP SDK旧凭据干扰
    _status = WIFI_DISCONNECTED;
}

bool wifiHasCredentials(void) {
    char ssid[33] = {0};
    return prefsGetString(PREF_WIFI_SSID, ssid, sizeof(ssid)) > 0;
}

static bool wifiWaitConnected(uint32_t timeoutMs) {
    uint32_t startTime = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (wifiIsAuthFailure(_lastDisconnectReason)) {
            Serial.printf("[WiFi] Auth failure, reason=%u (%s)\n",
                          _lastDisconnectReason,
                          WiFi.disconnectReasonName((wifi_err_reason_t)_lastDisconnectReason));
            return false;
        }
        if (millis() - startTime >= timeoutMs) {
            return false;
        }
        delay(100);
    }
    return true;
}

bool wifiConnect(uint32_t timeoutMs) {
    char ssid[33] = {0};
    char password[65] = {0};
    
    wifiApplyStaSettings();
    
    // 优先使用项目保存的凭据，避免ESP SDK里残留的旧密码导致AUTH_FAIL。
    if (prefsGetString(PREF_WIFI_SSID, ssid, sizeof(ssid)) > 0) {
        prefsGetString(PREF_WIFI_PASS, password, sizeof(password));
        Serial.printf("[WiFi] Connecting to %s... password length=%u\n", ssid, strlen(password));
        _lastDisconnectReason = 0;
        WiFi.disconnect(false, false);
        delay(100);
        WiFi.begin(ssid, password);
    } else if (WiFi.SSID().length() > 0) {
        Serial.printf("[WiFi] No NVS credentials, trying ESP stored SSID: %s\n", WiFi.SSID().c_str());
        _lastDisconnectReason = 0;
        WiFi.begin();
    } else {
        Serial.println("[WiFi] No saved credentials");
        _status = WIFI_CONNECTION_FAILED;
        return false;
    }

    _status = WIFI_CONNECTING;
    
    if (!wifiWaitConnected(timeoutMs)) {
        Serial.println("[WiFi] Connection timeout");
        _status = WIFI_CONNECTION_FAILED;
        return false;
    }
    
    Serial.printf("[WiFi] Connected, IP: %s\n", WiFi.localIP().toString().c_str());
    _status = WIFI_CONNECTED;
    
    // 保存成功的凭据到我们的存储
    if (ssid[0] != '\0') {
        wifiSaveCredentials(ssid, password);
    }
    
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

bool wifiStartConfigMode(const char* apName,
                         const char* apPass,
                         uint8_t timeoutMinutes,
                         WifiConfigPortalStartedCallback onStarted) {
    Serial.printf("[WiFi] Starting AP mode: %s\n", apName);

    _suppressDisconnectLog = true;
    WiFi.disconnect(true, true);  // 断开并清除ESP存储的凭据
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(500);
    WiFi.mode(WIFI_AP);
    delay(200);
    _suppressDisconnectLog = false;
    WiFi.setTxPower(WIFI_POWER_7dBm);
    esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_BW_HT20);
    esp_wifi_set_protocol(WIFI_IF_AP, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N);

    WiFiManager wm;
    wm.setDebugOutput(true);
    wm.setHostname(apName);
    wm.setWiFiAPChannel(1);
    wm.setConfigPortalBlocking(true);
    wm.setAPStaticIPConfig(
        IPAddress(192, 168, 4, 1),
        IPAddress(192, 168, 4, 1),
        IPAddress(255, 255, 255, 0)
    );
    wm.setConfigPortalTimeout(timeoutMinutes > 0 ? timeoutMinutes * 60 : 0);
    wm.setAPCallback([onStarted](WiFiManager*) {
        Serial.println("[WiFi] Config AP is active");
        Serial.printf("[WiFi] AP SSID: %s\n", WiFi.softAPSSID().c_str());
        Serial.printf("[WiFi] AP IP: %s\n", WiFi.softAPIP().toString().c_str());
        Serial.printf("[WiFi] AP channel: %d\n", WiFi.channel());
        Serial.printf("[WiFi] AP MAC: %s\n", WiFi.softAPmacAddress().c_str());
        Serial.printf("[WiFi] WiFi mode: %d\n", WiFi.getMode());
        Serial.printf("[WiFi] TX power: %d\n", WiFi.getTxPower());
        Serial.printf("[WiFi] Stations: %d\n", WiFi.softAPgetStationNum());
        if (onStarted) {
            onStarted();
        }
    });

    bool connected = wm.startConfigPortal(apName, strlen(apPass) > 0 ? apPass : nullptr);
    if (connected) {
        _status = WIFI_CONNECTED;
        Serial.printf("[WiFi] Config complete, IP: %s\n", WiFi.localIP().toString().c_str());
        char ssid[33] = {0};
        char password[65] = {0};
        if (wifiReadStaCredentials(ssid, sizeof(ssid), password, sizeof(password))) {
            Serial.printf("[WiFi] Saving configured SSID: %s, password length=%u\n", ssid, strlen(password));
            wifiSaveCredentials(ssid, password);
        } else if (WiFi.SSID().length() > 0) {
            Serial.printf("[WiFi] Saving connected SSID fallback: %s, password length=%u\n",
                          WiFi.SSID().c_str(), WiFi.psk().length());
            wifiSaveCredentials(WiFi.SSID().c_str(), WiFi.psk().c_str());
        }
        return true;
    }

    Serial.println("[WiFi] Config portal timeout or failed");
    if (timeoutMinutes > 0) {
        WiFi.mode(WIFI_OFF);
    }
    _status = WIFI_CONNECTION_FAILED;
    return false;
}

bool wifiStartConfigMode(const char* apName, const char* apPass, uint8_t timeoutMinutes) {
    return wifiStartConfigMode(apName, apPass, timeoutMinutes, nullptr);
}

void wifiSaveCredentials(const char* ssid, const char* password) {
    prefsPutString(PREF_WIFI_SSID, ssid);
    prefsPutString(PREF_WIFI_PASS, password);
}

void wifiClearCredentials(void) {
    prefsRemove(PREF_WIFI_SSID);
    prefsRemove(PREF_WIFI_PASS);
    WiFi.disconnect(true, true);  // 同时清除ESP存储的凭据
}

void wifiPowerOff(void) {
    WiFi.setAutoReconnect(false);
    WiFi.disconnect(false, false);
    delay(200);
    WiFi.mode(WIFI_OFF);
    _status = WIFI_DISCONNECTED;
    Serial.println("[WiFi] Powered off");
}
