/**
 * @file button.h
 * @brief Button input handling with OneButton library style
 * 
 * 按键处理 - 支持单击、双击、长按
 * 使用外部中断和 tick 机制
 */

#ifndef __BUTTON_H__
#define __BUTTON_H__

#include <Arduino.h>

// 按键事件回调类型
typedef void (*ButtonCallback)(void);

/**
 * @brief 初始化按键
 */
void buttonInit(void);

/**
 * @brief 设置单击回调
 * @param callback 回调函数
 */
void buttonAttachClick(ButtonCallback callback);

/**
 * @brief 设置双击回调
 * @param callback 回调函数
 */
void buttonAttachDoubleClick(ButtonCallback callback);

/**
 * @brief 设置长按回调
 * @param callback 回调函数
 */
void buttonAttachLongPress(ButtonCallback callback);

/**
 * @brief 按键 tick 处理 (需要在 loop 中调用)
 */
void buttonTick(void);

/**
 * @brief 获取按键当前状态
 * @return true 表示按键被按下
 */
bool buttonIsPressed(void);

/**
 * @brief 重置按键状态
 */
void buttonReset(void);

#endif // __BUTTON_H__
