/**
 * @file button.cpp
 * @brief Button input handling implementation
 */

#include "button.h"
#include "wiring.h"

// 按键状态机状态
typedef enum {
    BTN_STATE_IDLE = 0,
    BTN_STATE_PRESSED,
    BTN_STATE_RELEASED,
    BTN_STATE_WAIT_DOUBLE,
    BTN_STATE_LONG_PRESS
} ButtonState;

// 按键配置
#define DEBOUNCE_MS         50      // 消抖时间
#define CLICK_MS            300     // 单击最大时间
#define LONG_PRESS_MS       3000    // 长按判定时间
#define DOUBLE_CLICK_MS     400     // 双击间隔时间

// 静态变量
static ButtonState _state = BTN_STATE_IDLE;
static unsigned long _pressTime = 0;
static unsigned long _releaseTime = 0;
static uint8_t _clickCount = 0;

static ButtonCallback _clickCallback = nullptr;
static ButtonCallback _doubleClickCallback = nullptr;
static ButtonCallback _longPressCallback = nullptr;

// 内部函数：读取按键状态 (低电平有效)
static bool readButton(void) {
    return digitalRead(KEY_M) == LOW;
}

void buttonInit(void) {
    pinMode(KEY_M, INPUT_PULLUP);
    _state = BTN_STATE_IDLE;
    _pressTime = 0;
    _releaseTime = 0;
    _clickCount = 0;
}

void buttonAttachClick(ButtonCallback callback) {
    _clickCallback = callback;
}

void buttonAttachDoubleClick(ButtonCallback callback) {
    _doubleClickCallback = callback;
}

void buttonAttachLongPress(ButtonCallback callback) {
    _longPressCallback = callback;
}

void buttonTick(void) {
    bool pressed = readButton();
    unsigned long now = millis();
    
    switch (_state) {
        case BTN_STATE_IDLE:
            if (pressed) {
                _pressTime = now;
                _state = BTN_STATE_PRESSED;
            }
            break;
            
        case BTN_STATE_PRESSED:
            if (!pressed) {
                // 按键释放，检查是否为抖动
                if (now - _pressTime >= DEBOUNCE_MS) {
                    _releaseTime = now;
                    _clickCount++;
                    _state = BTN_STATE_WAIT_DOUBLE;
                } else {
                    _state = BTN_STATE_IDLE;
                }
            } else if (now - _pressTime >= LONG_PRESS_MS) {
                // 长按触发
                _state = BTN_STATE_LONG_PRESS;
                if (_longPressCallback) {
                    _longPressCallback();
                }
            }
            break;
            
        case BTN_STATE_WAIT_DOUBLE:
            if (pressed) {
                // 再次按下，检查是否在双击间隔内
                if (now - _releaseTime <= DOUBLE_CLICK_MS) {
                    _pressTime = now;
                    _state = BTN_STATE_PRESSED;
                } else {
                    // 超时，判定为单击
                    _state = BTN_STATE_PRESSED;
                    _clickCount = 1;
                }
            } else if (now - _releaseTime > DOUBLE_CLICK_MS) {
                // 双击间隔超时
                if (_clickCount == 1) {
                    if (_clickCallback) {
                        _clickCallback();
                    }
                } else if (_clickCount == 2) {
                    if (_doubleClickCallback) {
                        _doubleClickCallback();
                    }
                }
                _clickCount = 0;
                _state = BTN_STATE_IDLE;
            }
            break;
            
        case BTN_STATE_LONG_PRESS:
            if (!pressed) {
                // 长按释放
                _state = BTN_STATE_IDLE;
                _clickCount = 0;
            }
            break;
    }
}

bool buttonIsPressed(void) {
    return readButton();
}

void buttonReset(void) {
    _state = BTN_STATE_IDLE;
    _clickCount = 0;
}
