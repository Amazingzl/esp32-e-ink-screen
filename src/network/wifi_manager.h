/**
 * @file wifi_manager.h
 * @brief WiFi connection manager
 * 
 * WiFi 连接管理器
 * 支持自动重连和配置模式
 */

#ifndef __WIFI_MANAGER_H__
#define __WIFI_MANAGER_H__

#include <Arduino.h>

typedef void (*WifiConfigPortalStartedCallback)(void);

// WiFi 状态
typedef enum {
    WIFI_DISCONNECTED = 0,
    WIFI_CONNECTING,
    WIFI_CONNECTED,
    WIFI_CONNECTION_FAILED
} WifiStatus;

/**
 * @brief 初始化 WiFi 管理器
 */
void wifiInit(void);

/**
 * @brief 检查是否保存过 WiFi 凭据
 */
bool wifiHasCredentials(void);

/**
 * @brief 连接到已保存的 WiFi
 * @param timeoutMs 超时时间（毫秒）
 * @return true 连接成功
 */
bool wifiConnect(uint32_t timeoutMs = 10000);

/**
 * @brief 断开 WiFi 连接
 */
void wifiDisconnect(void);

/**
 * @brief 获取当前 WiFi 状态
 */
WifiStatus wifiGetStatus(void);

/**
 * @brief 检查是否已连接
 */
bool wifiIsConnected(void);

/**
 * @brief 获取信号强度 (RSSI)
 * @return 信号强度 dBm，0表示未连接
 */
int8_t wifiGetRSSI(void);

/**
 * @brief 获取本地 IP 地址
 * @return IP 地址字符串，未连接返回空字符串
 */
String wifiGetLocalIP(void);

/**
 * @brief 启动配置模式（AP模式）
 * @param apName AP名称
 * @param apPass AP密码
 * @param timeoutMinutes 超时时间（分钟），0表示不超时
 * @return true 配置完成
 */
bool wifiStartConfigMode(const char* apName, const char* apPass, uint8_t timeoutMinutes);

/**
 * @brief 启动配置模式，并在 AP 广播后回调
 */
bool wifiStartConfigMode(const char* apName,
                         const char* apPass,
                         uint8_t timeoutMinutes,
                         WifiConfigPortalStartedCallback onStarted);

/**
 * @brief 保存 WiFi 凭据
 */
void wifiSaveCredentials(const char* ssid, const char* password);

/**
 * @brief 清除 WiFi 凭据
 */
void wifiClearCredentials(void);

/**
 * @brief 关闭 WiFi 以节省电量
 */
void wifiPowerOff(void);

#endif // __WIFI_MANAGER_H__
