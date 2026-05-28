/**
 * @file wiring.h
 * @brief Hardware pin definitions for ESP32 C3
 * 
 * ESP32 C3 引脚定义
 * 注意：ESP32 C3 的 GPIO 引脚有限，需要合理分配
 */

#ifndef __WIRING_H__
#define __WIRING_H__

#include <Arduino.h>

// ============================================
// Board Selection
// ============================================
#define BOARD_ESP32_C3

#ifdef BOARD_ESP32_C3

// ============================================
// SPI Pins (E-Paper Display)
// ESP32 C3 SPI2 默认引脚
// ============================================
#define EPD_SPI_HOST    SPI2_HOST

// SPI 引脚 - 可根据实际接线修改
#define SPI_MOSI        GPIO_NUM_5   // SDI/MOSI - 主输出从输入
#define SPI_SCK         GPIO_NUM_4   // SCK - 时钟
#define SPI_CS          GPIO_NUM_6   // CS - 片选
#define SPI_DC          GPIO_NUM_7   // DC - 数据/命令选择
#define SPI_RST         GPIO_NUM_8   // RST - 复位
#define SPI_BUSY        GPIO_NUM_3   // BUSY - 忙信号，避开 ESP32-C3 启动/Flash 相关 GPIO9

// ============================================
// Input Pins
// ============================================
// 功能按键 - 必须使用 RTC GPIO 支持深度休眠唤醒
// ESP32 C3 支持 RTC 唤醒的 GPIO: 0-5
#define KEY_M           GPIO_NUM_2   // 主功能按键 (GPIO2 支持 RTC)

// ============================================
// Output Pins
// ============================================
#define PIN_LED         GPIO_NUM_10  // 状态 LED

// ============================================
// ADC Pins
// ============================================
// ESP32 C3 ADC1 通道: GPIO0-GPIO4
#define PIN_ADC         GPIO_NUM_0   // 电池电压检测

// ============================================
// I2C Pins (Optional - for sensors)
// 当前墨水屏接线占用 GPIO3/GPIO4；如需 I2C 传感器，需要重新分配空闲引脚。
// ============================================
#define I2C_SDA         GPIO_NUM_3   // I2C 数据 (与 SPI_BUSY 共享，不能同时使用)
#define I2C_SCL         GPIO_NUM_4   // I2C 时钟 (与 SPI_SCK 共享，不能同时使用)

// ============================================
// UART Pins (Debug)
// ============================================
// UART0: GPIO20 (RX), GPIO21 (TX) - USB 串口

#endif // BOARD_ESP32_C3

// ============================================
// Constants
// ============================================
#define BATTERY_ADC_DIVIDER     2.0f    // 分压电阻比例 (如果使用了分压电路)
#define BATTERY_MAX_VOLTAGE     4200    // 锂电池最大电压 mV
#define BATTERY_MIN_VOLTAGE     3000    // 锂电池最小电压 mV
#define BATTERY_WARNING_VOLTAGE 3300    // 低电量警告电压 mV

// 按键配置
#define KEY_CLICK_MS            300     // 单击判定时间 ms
#define KEY_LONG_PRESS_MS       3000    // 长按判定时间 ms
#define KEY_DOUBLE_CLICK_MS     400     // 双击间隔 ms

// 休眠时间配置
#define SLEEP_DURATION_MINUTES  30      // 默认休眠时长（分钟）

#endif // __WIRING_H__
