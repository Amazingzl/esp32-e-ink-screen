/**
 * @file display_manager.h
 * @brief Display manager with partial update support
 * 
 * 显示管理器 - 支持局部刷新
 * 封装 GxEPD2 的高级功能，简化使用
 */

#ifndef __DISPLAY_MANAGER_H__
#define __DISPLAY_MANAGER_H__

#include <Arduino.h>
#include "epaper_driver.h"

// ============================================
// Page Height for Paged Drawing
// ============================================
#define PAGE_HEIGHT     16  // 分页高度，影响内存使用

// ============================================
// Display Manager Class
// ============================================
class DisplayManager {
public:
    DisplayManager();
    
    /**
     * @brief 初始化显示管理器
     */
    void init();
    
    // ========================================
    // Window Management (Partial Update)
    // ========================================
    
    /**
     * @brief 设置全屏刷新窗口
     */
    void setFullWindow();
    
    /**
     * @brief 设置局部刷新窗口
     * @param x 起始X坐标 (必须是8的倍数)
     * @param y 起始Y坐标
     * @param w 宽度 (必须是8的倍数)
     * @param h 高度
     * 
     * 注意：x和w必须是8的倍数，这是墨水屏控制器的寻址限制
     */
    void setPartialWindow(int16_t x, int16_t y, int16_t w, int16_t h);
    
    /**
     * @brief 检查当前是否为局部刷新模式
     */
    bool isPartialMode();
    
    // ========================================
    // Drawing Operations
    // ========================================
    
    /**
     * @brief 开始绘制（分页模式）
     */
    void firstPage();
    
    /**
     * @brief 下一页
     * @return false 表示所有页面绘制完成
     */
    bool nextPage();
    
    /**
     * @brief 清屏
     * @param color 颜色 (COLOR_BLACK, COLOR_WHITE, COLOR_RED)
     */
    void fillScreen(uint16_t color);
    
    /**
     * @brief 绘制像素
     */
    void drawPixel(int16_t x, int16_t y, uint16_t color);
    
    /**
     * @brief 绘制直线
     */
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
    
    /**
     * @brief 绘制矩形（边框）
     */
    void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    
    /**
     * @brief 绘制填充矩形
     */
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    
    /**
     * @brief 绘制圆
     */
    void drawCircle(int16_t x, int16_t y, int16_t r, uint16_t color);
    
    /**
     * @brief 绘制填充圆
     */
    void fillCircle(int16_t x, int16_t y, int16_t r, uint16_t color);
    
    /**
     * @brief 绘制三角形
     */
    void drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, 
                      int16_t x2, int16_t y2, uint16_t color);
    
    // ========================================
    // Text Operations
    // ========================================
    
    /**
     * @brief 设置字体
     * @param font Adafruit_GFX 字体
     */
    void setFont(const GFXfont* font);
    
    /**
     * @brief 设置文本颜色
     */
    void setTextColor(uint16_t color);
    
    /**
     * @brief 设置文本颜色（带背景色）
     */
    void setTextColor(uint16_t color, uint16_t background);
    
    /**
     * @brief 设置文本大小
     */
    void setTextSize(uint8_t size);
    
    /**
     * @brief 设置光标位置
     */
    void setCursor(int16_t x, int16_t y);
    
    /**
     * @brief 打印文本
     */
    void print(const char* text);
    void print(String text);
    void print(int num);
    void print(float num, int digits = 2);
    
    /**
     * @brief 获取文本边界框
     */
    void getTextBounds(const char* text, int16_t x, int16_t y,
                       int16_t* x1, int16_t* y1, uint16_t* w, uint16_t* h);
    
    // ========================================
    // Refresh Operations
    // ========================================
    
    /**
     * @brief 全屏刷新
     */
    void fullRefresh();
    
    /**
     * @brief 局部刷新（当前窗口）
     */
    void partialRefresh();
    
    /**
     * @brief 刷新指定区域
     * @param x 起始X坐标
     * @param y 起始Y坐标
     * @param w 宽度
     * @param h 高度
     */
    void refreshArea(int16_t x, int16_t y, int16_t w, int16_t h);
    
    // ========================================
    // Power Management
    // ========================================
    
    /**
     * @brief 关闭电源
     */
    void powerOff();
    
    /**
     * @brief 进入休眠
     */
    void hibernate();
    
    // ========================================
    // Utilities
    // ========================================
    
    /**
     * @brief 获取屏幕宽度
     */
    int16_t width();
    
    /**
     * @brief 获取屏幕高度
     */
    int16_t height();
    
    /**
     * @brief 设置旋转方向
     * @param rotation 0-3 (0度, 90度, 180度, 270度)
     */
    void setRotation(uint8_t rotation);
    
    /**
     * @brief 获取当前旋转方向
     */
    uint8_t getRotation();
    
    /**
     * @brief 对齐坐标到8的倍数（满足控制器要求）
     * @param value 输入值
     * @return 对齐后的值
     */
    static int16_t alignTo8(int16_t value);

private:
    bool _initialized;
    bool _inPartialMode;
};

// 全局显示管理器实例
extern DisplayManager displayManager;

#endif // __DISPLAY_MANAGER_H__
