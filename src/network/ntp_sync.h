/**
 * @file ntp_sync.h
 * @brief NTP time synchronization
 * 
 * NTP 时间同步
 */

#ifndef __NTP_SYNC_H__
#define __NTP_SYNC_H__

#include <Arduino.h>
#include <time.h>

// NTP 服务器 - 使用多个国内服务器提高成功率
#define NTP_SERVER_1    "ntp.aliyun.com"
#define NTP_SERVER_2    "ntp.tencent.com"
#define NTP_SERVER_3    "cn.pool.ntp.org"

// 默认时区（中国东八区）
// POSIX 时区格式: 标准时区名+偏移量[夏令时规则]
// CST-8 表示中国标准时间，比 UTC 快 8 小时
#define DEFAULT_TIMEZONE    "CST-8"

/**
 * @brief 初始化时间同步
 */
void ntpInit(void);

/**
 * @brief 同步 NTP 时间
 * @param timeoutMs 超时时间（毫秒）
 * @return true 同步成功
 */
bool ntpSync(uint32_t timeoutMs = 5000);

/**
 * @brief 检查时间是否已同步
 */
bool ntpIsSynced(void);

/**
 * @brief 获取当前时间结构体
 */
struct tm ntpGetTime(void);

/**
 * @brief 获取当前时间戳
 */
time_t ntpGetTimestamp(void);

/**
 * @brief 格式化时间为字符串
 * @param format 格式字符串，如 "%Y-%m-%d %H:%M:%S"
 * @return 格式化后的时间字符串
 */
String ntpFormatTime(const char* format);

/**
 * @brief 获取当前年份
 */
int ntpGetYear(void);

/**
 * @brief 获取当前月份 (1-12)
 */
int ntpGetMonth(void);

/**
 * @brief 获取当前日期 (1-31)
 */
int ntpGetDay(void);

/**
 * @brief 获取当前小时 (0-23)
 */
int ntpGetHour(void);

/**
 * @brief 获取当前分钟 (0-59)
 */
int ntpGetMinute(void);

/**
 * @brief 获取当前秒 (0-59)
 */
int ntpGetSecond(void);

/**
 * @brief 获取星期几 (0=周日, 1=周一, ...)
 */
int ntpGetWeekday(void);

#endif // __NTP_SYNC_H__
