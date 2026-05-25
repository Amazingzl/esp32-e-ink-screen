/**
 * @file power_manager.h
 * @brief Power management for deep sleep
 * 
 * 电源管理 - 深度休眠控制
 */

#ifndef __POWER_MANAGER_H__
#define __POWER_MANAGER_H__

#include <Arduino.h>

// 唤醒原因
typedef enum {
    WAKEUP_REASON_POWER_ON = 0,     // 上电启动
    WAKEUP_REASON_EXT0,              // 外部中断 EXT0
    WAKEUP_REASON_EXT1,              // 外部中断 EXT1
    WAKEUP_REASON_TIMER,             // 定时器唤醒
    WAKEUP_REASON_TOUCH,             // 触摸唤醒
    WAKEUP_REASON_ULP,               // ULP 唤醒
    WAKEUP_REASON_UNKNOWN            // 未知原因
} WakeupReason;

/**
 * @brief 初始化电源管理
 */
void powerInit(void);

/**
 * @brief 进入深度休眠
 * @param sleepMinutes 休眠时长（分钟）
 * 
 * 休眠期间：
 * - CPU 停止运行
 * - RAM 数据保持（如果配置）
 * - 可由 GPIO 中断或定时器唤醒
 */
void powerDeepSleep(uint32_t sleepMinutes);

/**
 * @brief 进入轻度休眠
 * @param sleepMs 休眠时长（毫秒）
 */
void powerLightSleep(uint32_t sleepMs);

/**
 * @brief 获取唤醒原因
 */
WakeupReason powerGetWakeupReason(void);

/**
 * @brief 打印唤醒原因
 */
void powerPrintWakeupReason(void);

/**
 * @brief 配置 GPIO 唤醒
 * @param gpioNum GPIO 编号
 * @param level 触发电平 (0=低电平, 1=高电平)
 * 
 * 注意：ESP32 C3 只有部分 GPIO 支持 RTC 唤醒
 */
void powerEnableGpioWakeup(uint8_t gpioNum, uint8_t level);

/**
 * @brief 配置定时器唤醒
 * @param minutes 分钟数
 */
void powerEnableTimerWakeup(uint32_t minutes);

/**
 * @brief 禁用所有唤醒源
 */
void powerDisableAllWakeup(void);

#endif // __POWER_MANAGER_H__
