/**
 * @file epaper_driver.h
 * @brief E-Paper display driver wrapper for GxEPD2
 * 
 * 墨水屏驱动封装
 * 支持多种屏幕型号，通过编译宏选择
 */

#ifndef __EPAPER_DRIVER_H__
#define __EPAPER_DRIVER_H__

#include <Arduino.h>
#include <GxEPD2_3C.h>
#include <GxEPD2_BW.h>
#include "hardware/wiring.h"

// ============================================
// Display Driver Selection
// ============================================
// 根据 platformio.ini 中的定义选择驱动

#ifndef EPD_DRIVER
#define EPD_DRIVER GxEPD2_420c_Z21
#endif

// 屏幕参数定义
#if EPD_DRIVER == GxEPD2_420c
    #define SCREEN_WIDTH    400
    #define SCREEN_HEIGHT   300
    #define SCREEN_COLORS   3
    #define HAS_PARTIAL     true
    #define HAS_FAST_PARTIAL false
#elif EPD_DRIVER == GxEPD2_420c_Z21
    #define SCREEN_WIDTH    400
    #define SCREEN_HEIGHT   300
    #define SCREEN_COLORS   3
    #define HAS_PARTIAL     true
    #define HAS_FAST_PARTIAL false
#elif EPD_DRIVER == GxEPD2_420c_GDEY042Z98
    #define SCREEN_WIDTH    400
    #define SCREEN_HEIGHT   300
    #define SCREEN_COLORS   3
    #define HAS_PARTIAL     true
    #define HAS_FAST_PARTIAL false
#elif EPD_DRIVER == GxEPD2_420
    #define SCREEN_WIDTH    400
    #define SCREEN_HEIGHT   300
    #define SCREEN_COLORS   2
    #define HAS_PARTIAL     true
    #define HAS_FAST_PARTIAL true
#else
    // 默认使用 4.2寸三色屏
    #define SCREEN_WIDTH    400
    #define SCREEN_HEIGHT   300
    #define SCREEN_COLORS   3
    #define HAS_PARTIAL     true
    #define HAS_FAST_PARTIAL false
#endif

// ============================================
// Display Instance
// ============================================
#if SCREEN_COLORS == 3
    // 三色屏 (黑、白、红/黄)
    extern GxEPD2_3C<EPD_DRIVER, EPD_DRIVER::HEIGHT> display;
#else
    // 黑白屏
    extern GxEPD2_BW<EPD_DRIVER, EPD_DRIVER::HEIGHT> display;
#endif

// ============================================
// Color Definitions
// ============================================
#define COLOR_BLACK     GxEPD_BLACK
#define COLOR_WHITE     GxEPD_WHITE
#define COLOR_RED       GxEPD_RED
#define COLOR_YELLOW    GxEPD_YELLOW

// ============================================
// Functions
// ============================================

/**
 * @brief 初始化墨水屏
 * @param serial_diag_bitrate 串口诊断波特率，0表示禁用
 */
void epaperInit(uint32_t serial_diag_bitrate = 0);

/**
 * @brief 重新初始化（从深度休眠唤醒后使用）
 * @param initial false 表示不是首次初始化
 */
void epaperReinit(bool initial = false);

/**
 * @brief 关闭墨水屏电源
 */
void epaperPowerOff(void);

/**
 * @brief 进入休眠模式
 */
void epaperHibernate(void);

/**
 * @brief 检查是否支持局部刷新
 * @return true 支持局部刷新
 */
bool epaperHasPartialUpdate(void);

/**
 * @brief 检查是否支持快速局部刷新
 * @return true 支持快速局部刷新
 */
bool epaperHasFastPartialUpdate(void);

/**
 * @brief 获取屏幕宽度
 */
int16_t epaperWidth(void);

/**
 * @brief 获取屏幕高度
 */
int16_t epaperHeight(void);

#endif // __EPAPER_DRIVER_H__
