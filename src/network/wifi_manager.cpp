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

static const char* wifiAuthModeName(wifi_auth_mode_t authMode) {
    switch (authMode) {
        case WIFI_AUTH_OPEN: return "OPEN";
        case WIFI_AUTH_WEP: return "WEP";
        case WIFI_AUTH_WPA_PSK: return "WPA";
        case WIFI_AUTH_WPA2_PSK: return "WPA2";
        case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2";
        case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2-ENT";
        case WIFI_AUTH_WPA3_PSK: return "WPA3";
        case WIFI_AUTH_WPA2_WPA3_PSK: return "WPA2/WPA3";
        default: return "UNKNOWN";
    }
}

static bool wifiFindBestAp(const char* targetSsid, int32_t* bestChannel, uint8_t bestBssid[6]) {
    Serial.printf("[WiFi] Scanning for SSID: %s\n", targetSsid);

    int32_t networkCount = WiFi.scanNetworks(false, true);
    if (networkCount <= 0) {
        Serial.printf("[WiFi] Scan found no networks (%ld)\n", networkCount);
        WiFi.scanDelete();
        return false;
    }

    int32_t bestRssi = -128;
    bool found = false;

    for (int32_t i = 0; i < networkCount; i++) {
        if (WiFi.SSID(i) != targetSsid) {
            continue;
        }

        found = true;
        int32_t rssi = WiFi.RSSI(i);
        int32_t channel = WiFi.channel(i);
        wifi_auth_mode_t authMode = WiFi.encryptionType(i);

        Serial.printf("[WiFi] Found target: RSSI=%ld dBm, channel=%ld, auth=%s, BSSID=%s\n",
                      rssi, channel, wifiAuthModeName(authMode), WiFi.BSSIDstr(i).c_str());

        if (rssi > bestRssi) {
            bestRssi = rssi;
            *bestChannel = channel;
            memcpy(bestBssid, WiFi.BSSID(i), 6);
        }
    }

    if (!found) {
        Serial.printf("[WiFi] Target SSID not found. Nearby networks: %ld\n", networkCount);
    }

    WiFi.scanDelete();
    return found;
}

static void wifiApplyStaSettings(void) {
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.setTxPower(WIFI_POWER_19_5dBm);
    esp_wifi_set_ps(WIFI_PS_NONE);
    esp_wifi_set_bandwidth(WIFI_IF_STA, WIFI_BW_HT20);
    esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N);
}

void wifiInit(void) {
    wifiApplyStaSettings();
    WiFi.setAutoReconnect(true);  // 启用自动重连
    WiFi.persistent(false);       // 使用项目自己的NVS凭据，避免ESP SDK旧凭据干扰
    _status = WIFI_DISCONNECTED;
}

static bool wifiWaitConnected(uint32_t timeoutMs) {
    uint32_t startTime = millis();
    while (WiFi.status() != WL_CONNECTED) {
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
    int32_t channel = 0;
    uint8_t bssid[6] = {0};
    bool hasTargetAp = false;
    
    wifiApplyStaSettings();
    
    // 优先使用项目保存的凭据，避免ESP SDK里残留的旧密码导致AUTH_FAIL。
    if (prefsGetString(PREF_WIFI_SSID, ssid, sizeof(ssid)) > 0) {
        prefsGetString(PREF_WIFI_PASS, password, sizeof(password));
        Serial.printf("[WiFi] Connecting to %s... password length=%u\n", ssid, strlen(password));
        hasTargetAp = wifiFindBestAp(ssid, &channel, bssid);
        WiFi.disconnect(false, false);
        delay(100);
        if (hasTargetAp && channel > 0) {
            Serial.printf("[WiFi] Connecting on scanned channel %ld and BSSID %02X:%02X:%02X:%02X:%02X:%02X\n",
                          channel, bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
            WiFi.begin(ssid, password, channel, bssid);
        } else {
            WiFi.begin(ssid, password);
        }
    } else if (WiFi.SSID().length() > 0) {
        Serial.printf("[WiFi] No NVS credentials, trying ESP stored SSID: %s\n", WiFi.SSID().c_str());
        WiFi.begin();
    } else {
        Serial.println("[WiFi] No saved credentials");
        _status = WIFI_CONNECTION_FAILED;
        return false;
    }

    _status = WIFI_CONNECTING;
    
    if (!wifiWaitConnected(timeoutMs)) {
        if (ssid[0] != '\0' && hasTargetAp) {
            Serial.println("[WiFi] BSSID connection timeout, retrying without fixed BSSID");
            WiFi.disconnect(false, false);
            delay(500);
            wifiApplyStaSettings();
            WiFi.begin(ssid, password);

            if (!wifiWaitConnected(timeoutMs)) {
                Serial.println("[WiFi] Connection timeout");
                _status = WIFI_CONNECTION_FAILED;
                return false;
            }
        } else {
            Serial.println("[WiFi] Connection timeout");
            _status = WIFI_CONNECTION_FAILED;
            return false;
        }
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

bool wifiStartConfigMode(const char* apName, const char* apPass, uint8_t timeoutMinutes) {
    Serial.printf("[WiFi] Starting AP mode: %s\n", apName);

    WiFi.disconnect(true, true);  // 断开并清除ESP存储的凭据
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(500);
    WiFi.mode(WIFI_AP);
    delay(200);
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
