/**
 * @file preferences.h
 * @brief NVS preferences wrapper for configuration storage
 * 
 * 配置存储封装 - 使用 ESP32 NVS
 */

#ifndef __PREFERENCES_H__
#define __PREFERENCES_H__

#include <Arduino.h>

// 命名空间
#define PREF_NAMESPACE      "epaper_app"

// 配置键名
#define PREF_WIFI_SSID      "wifi_ssid"
#define PREF_WIFI_PASS      "wifi_pass"
#define PREF_TIMEZONE       "timezone"
#define PREF_LAST_UPDATE    "last_update"
#define PREF_SCREEN_ROT     "screen_rot"

/**
 * @brief 初始化配置存储
 */
void prefsInit(void);

/**
 * @brief 保存字符串配置
 */
bool prefsPutString(const char* key, const char* value);

/**
 * @brief 获取字符串配置
 * @param key 键名
 * @param buffer 缓冲区
 * @param maxLen 最大长度
 * @return 实际长度，0表示不存在
 */
size_t prefsGetString(const char* key, char* buffer, size_t maxLen);

/**
 * @brief 保存整数配置
 */
bool prefsPutInt(const char* key, int32_t value);

/**
 * @brief 获取整数配置
 * @param key 键名
 * @param defaultValue 默认值
 */
int32_t prefsGetInt(const char* key, int32_t defaultValue = 0);

/**
 * @brief 保存布尔配置
 */
bool prefsPutBool(const char* key, bool value);

/**
 * @brief 获取布尔配置
 */
bool prefsGetBool(const char* key, bool defaultValue = false);

/**
 * @brief 检查键是否存在
 */
bool prefsExists(const char* key);

/**
 * @brief 删除配置项
 */
bool prefsRemove(const char* key);

/**
 * @brief 清除所有配置
 */
bool prefsClear(void);

#endif // __PREFERENCES_H__
