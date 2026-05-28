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

void wifiInit(void) {
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(false);  // 我们手动控制重连
    _status = WIFI_DISCONNECTED;
}

bool wifiConnect(uint32_t timeoutMs) {
    char ssid[33] = {0};
    char password[65] = {0};
    
    WiFi.mode(WIFI_STA);

    // Prefer app-managed credentials, then fall back to credentials stored by WiFiManager/ESP.
    if (prefsGetString(PREF_WIFI_SSID, ssid, sizeof(ssid)) > 0) {
        prefsGetString(PREF_WIFI_PASS, password, sizeof(password));
        Serial.printf("[WiFi] Connecting to %s...\n", ssid);
        WiFi.begin(ssid, password);
    } else {
        Serial.println("[WiFi] Connecting with stored ESP credentials...");
        WiFi.begin();
    }

    _status = WIFI_CONNECTING;
    
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

    WiFi.disconnect(true, false);
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(500);
    WiFi.mode(WIFI_AP);
    delay(200);
    WiFi.setTxPower(WIFI_POWER_19_5dBm);
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
    wm.setAPCallback([](WiFiManager*) {
        Serial.println("[WiFi] Config AP is active");
        Serial.printf("[WiFi] AP SSID: %s\n", WiFi.softAPSSID().c_str());
        Serial.printf("[WiFi] AP IP: %s\n", WiFi.softAPIP().toString().c_str());
        Serial.printf("[WiFi] AP channel: %d\n", WiFi.channel());
        Serial.printf("[WiFi] AP MAC: %s\n", WiFi.softAPmacAddress().c_str());
        Serial.printf("[WiFi] WiFi mode: %d\n", WiFi.getMode());
        Serial.printf("[WiFi] TX power: %d\n", WiFi.getTxPower());
        Serial.printf("[WiFi] Stations: %d\n", WiFi.softAPgetStationNum());
    });

    bool connected = wm.startConfigPortal(apName, strlen(apPass) > 0 ? apPass : nullptr);
    if (connected) {
        _status = WIFI_CONNECTED;
        Serial.printf("[WiFi] Config complete, IP: %s\n", WiFi.localIP().toString().c_str());
        if (WiFi.SSID().length() > 0) {
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
