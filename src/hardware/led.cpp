/**
 * @file led.cpp
 * @brief LED indicator control implementation
 */

#include "led.h"
#include "wiring.h"

static LedState _currentState = LED_OFF;
static unsigned long _lastBlinkTime = 0;
static uint8_t _blinkCount = 0;
static bool _ledState = false;

// 闪烁间隔配置 (ms)
#define FAST_BLINK_INTERVAL     250     // 快闪间隔
#define SLOW_BLINK_INTERVAL     1000    // 慢闪间隔
#define TRIPLE_BLINK_ON         100     // 三短闪亮时间
#define TRIPLE_BLINK_OFF        100     // 三短闪灭时间
#define TRIPLE_BLINK_PAUSE      500     // 三短闪间隔

void ledInit(void) {
    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_LED, LOW);
    _currentState = LED_OFF;
    _lastBlinkTime = 0;
    _blinkCount = 0;
    _ledState = false;
}

void ledSetState(LedState state) {
    if (_currentState != state) {
        _currentState = state;
        _lastBlinkTime = millis();
        _blinkCount = 0;
        
        switch (state) {
            case LED_ON:
                digitalWrite(PIN_LED, HIGH);
                _ledState = true;
                break;
            case LED_OFF:
                digitalWrite(PIN_LED, LOW);
                _ledState = false;
                break;
            default:
                // 闪烁模式由 loop 处理
                break;
        }
    }
}

void ledOn(void) {
    ledSetState(LED_ON);
}

void ledOff(void) {
    ledSetState(LED_OFF);
}

void ledFastBlink(void) {
    ledSetState(LED_FAST_BLINK);
}

void ledSlowBlink(void) {
    ledSetState(LED_SLOW_BLINK);
}

void ledTripleBlink(void) {
    ledSetState(LED_TRIPLE_BLINK);
}

void ledLoop(void) {
    unsigned long currentTime = millis();
    
    switch (_currentState) {
        case LED_FAST_BLINK:
            if (currentTime - _lastBlinkTime >= FAST_BLINK_INTERVAL) {
                _lastBlinkTime = currentTime;
                _ledState = !_ledState;
                digitalWrite(PIN_LED, _ledState ? HIGH : LOW);
            }
            break;
            
        case LED_SLOW_BLINK:
            if (currentTime - _lastBlinkTime >= SLOW_BLINK_INTERVAL) {
                _lastBlinkTime = currentTime;
                _ledState = !_ledState;
                digitalWrite(PIN_LED, _ledState ? HIGH : LOW);
            }
            break;
            
        case LED_TRIPLE_BLINK:
            {
                unsigned long elapsed = currentTime - _lastBlinkTime;
                uint8_t phase = _blinkCount % 6; // 0-5: 亮灭亮灭亮灭  pause
                
                if (phase < 6) {
                    // 三短闪阶段
                    if (phase % 2 == 0) {
                        // 亮
                        if (!_ledState) {
                            digitalWrite(PIN_LED, HIGH);
                            _ledState = true;
                        }
                        if (elapsed >= TRIPLE_BLINK_ON) {
                            _lastBlinkTime = currentTime;
                            _blinkCount++;
                        }
                    } else {
                        // 灭
                        if (_ledState) {
                            digitalWrite(PIN_LED, LOW);
                            _ledState = false;
                        }
                        if (elapsed >= TRIPLE_BLINK_OFF) {
                            _lastBlinkTime = currentTime;
                            _blinkCount++;
                        }
                    }
                } else {
                    // 暂停阶段
                    if (_ledState) {
                        digitalWrite(PIN_LED, LOW);
                        _ledState = false;
                    }
                    if (elapsed >= TRIPLE_BLINK_PAUSE) {
                        _lastBlinkTime = currentTime;
                        _blinkCount = 0;
                    }
                }
            }
            break;
            
        default:
            break;
    }
}
