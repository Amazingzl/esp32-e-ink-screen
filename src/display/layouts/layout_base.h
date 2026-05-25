/**
 * @file layout_base.h
 * @brief Base class for display layouts
 * 
 * 布局基类 - 所有屏幕布局的抽象基类
 */

#ifndef __LAYOUT_BASE_H__
#define __LAYOUT_BASE_H__

#include <Arduino.h>
#include "../display_manager.h"

/**
 * @brief 布局基类
 * 所有具体布局（日历、时钟、天气等）都继承此类
 */
class LayoutBase {
public:
    LayoutBase() {}
    virtual ~LayoutBase() {}
    
    /**
     * @brief 初始化布局
     */
    virtual void init() = 0;
    
    /**
     * @brief 绘制完整布局（全屏刷新）
     */
    virtual void draw() = 0;
    
    /**
     * @brief 更新局部区域
     * @param area 区域编号，由具体布局定义
     */
    virtual void updatePartial(uint8_t area) = 0;
    
    /**
     * @brief 获取布局名称
     */
    virtual const char* getName() = 0;
    
    /**
     * @brief 处理按键事件
     * @param clickType 0=单击, 1=双击, 2=长按
     */
    virtual void onButton(uint8_t clickType) = 0;
};

#endif // __LAYOUT_BASE_H__
