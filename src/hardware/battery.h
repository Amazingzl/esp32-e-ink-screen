/**
 * @file battery.h
 * @brief Battery voltage monitoring
 * 
 * 电池电压监测
 * 使用 ADC 读取电池电压（通过分压电阻）
 */

#ifndef __BATTERY_H__
#define __BATTERY_H__

#include <Arduino.h>

/**
 * @brief 电池状态枚举
 */
typedef enum {
    BATTERY_OK = 0,         // 电量正常
    BATTERY_LOW,            // 电量低
    BATTERY_CRITICAL,       // 电量严重不足
    BATTERY_NO_BATTERY      // 未检测到电池
} BatteryStatus;

/**
 * @brief 初始化电池检测
 */
void batteryInit(void);

/**
 * @brief 读取电池电压 (mV)
 * @return 电池电压，单位毫伏
 */
uint16_t batteryReadVoltage(void);

/**
 * @brief 获取电池状态
 * @return 电池状态
 */
BatteryStatus batteryGetStatus(void);

/**
 * @brief 检查电池是否需要警告
 * @return true 表示电量低需要警告
 */
bool batteryNeedWarning(void);

/**
 * @brief 获取电池电量百分比
 * @return 电量百分比 0-100
 */
uint8_t batteryGetPercentage(void);

/**
 * @brief 打印电池信息到串口
 */
void batteryPrintInfo(void);

#endif // __BATTERY_H__
