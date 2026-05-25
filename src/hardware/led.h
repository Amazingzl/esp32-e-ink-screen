/**
 * @file led.h
 * @brief LED indicator control
 * 
 * LED 状态指示器
 * 快闪: 系统启动中/WiFi连接中
 * 常亮: WiFi连接成功
 * 慢闪: WiFi连接失败
 * 三短闪: 配置模式
 * 熄灭: 系统休眠
 */

#ifndef __LED_H__
#define __LED_H__

#include <Arduino.h>

// LED 状态枚举
typedef enum {
    LED_OFF = 0,           // 熄灭
    LED_ON,                // 常亮
    LED_FAST_BLINK,        // 快闪 (约2Hz)
    LED_SLOW_BLINK,        // 慢闪 (约0.5Hz)
    LED_TRIPLE_BLINK,      // 三短闪
} LedState;

/**
 * @brief 初始化 LED
 */
void ledInit(void);

/**
 * @brief 设置 LED 状态
 * @param state LED 状态
 */
void ledSetState(LedState state);

/**
 * @brief LED 常亮
 */
void ledOn(void);

/**
 * @brief LED 熄灭
 */
void ledOff(void);

/**
 * @brief LED 快闪 (系统启动/连接中)
 */
void ledFastBlink(void);

/**
 * @brief LED 慢闪 (连接失败)
 */
void ledSlowBlink(void);

/**
 * @brief LED 三短闪 (配置模式)
 */
void ledTripleBlink(void);

/**
 * @brief LED 处理函数 (需要在 loop 中调用)
 */
void ledLoop(void);

#endif // __LED_H__
