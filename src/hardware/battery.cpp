/**
 * @file battery.cpp
 * @brief Battery voltage monitoring implementation
 */

#include "battery.h"
#include "wiring.h"

// ADC 配置
#define ADC_ATTENUATION     ADC_11db    // 11dB 衰减，可测量 0-3.3V
#define ADC_RESOLUTION      12          // 12位分辨率 (0-4095)

// 分压电阻配置 (如果使用了分压电路)
// 例如：两个 100K 电阻分压，比例为 0.5
#define VOLTAGE_DIVIDER_RATIO   2.0f

// ADC 参考电压
#define ADC_REFERENCE_VOLTAGE   3300    // mV (ESP32 C3 默认 3.3V)

void batteryInit(void) {
    analogReadResolution(ADC_RESOLUTION);
    analogSetAttenuation(ADC_ATTENUATION);
    pinMode(PIN_ADC, INPUT);
}

uint16_t batteryReadVoltage(void) {
    // 多次采样取平均，提高精度
    const int samples = 10;
    int32_t sum = 0;
    
    for (int i = 0; i < samples; i++) {
        sum += analogRead(PIN_ADC);
        delayMicroseconds(100);
    }
    
    int average = sum / samples;
    
    // 计算电压: ADC值 / 最大值 * 参考电压 * 分压比例
    uint16_t voltage = (uint16_t)((average * (uint32_t)ADC_REFERENCE_VOLTAGE * VOLTAGE_DIVIDER_RATIO) 
                                   / ((1 << ADC_RESOLUTION) - 1));
    
    return voltage;
}

BatteryStatus batteryGetStatus(void) {
    uint16_t voltage = batteryReadVoltage();
    
    if (voltage < 2500) {
        return BATTERY_NO_BATTERY;  // 可能未接电池或ADC电路异常
    } else if (voltage < BATTERY_MIN_VOLTAGE) {
        return BATTERY_CRITICAL;
    } else if (voltage < BATTERY_WARNING_VOLTAGE) {
        return BATTERY_LOW;
    } else {
        return BATTERY_OK;
    }
}

bool batteryNeedWarning(void) {
    BatteryStatus status = batteryGetStatus();
    return (status == BATTERY_LOW || status == BATTERY_CRITICAL);
}

uint8_t batteryGetPercentage(void) {
    uint16_t voltage = batteryReadVoltage();
    
    if (voltage >= BATTERY_MAX_VOLTAGE) {
        return 100;
    } else if (voltage <= BATTERY_MIN_VOLTAGE) {
        return 0;
    } else {
        // 线性计算电量百分比
        return (uint8_t)(((voltage - BATTERY_MIN_VOLTAGE) * 100) 
                         / (BATTERY_MAX_VOLTAGE - BATTERY_MIN_VOLTAGE));
    }
}

void batteryPrintInfo(void) {
    uint16_t voltage = batteryReadVoltage();
    uint8_t percentage = batteryGetPercentage();
    BatteryStatus status = batteryGetStatus();
    
    Serial.println("===== Battery Info =====");
    Serial.printf("Voltage: %d mV\n", voltage);
    Serial.printf("Percentage: %d%%\n", percentage);
    
    switch (status) {
        case BATTERY_OK:
            Serial.println("Status: OK");
            break;
        case BATTERY_LOW:
            Serial.println("Status: LOW");
            break;
        case BATTERY_CRITICAL:
            Serial.println("Status: CRITICAL");
            break;
        case BATTERY_NO_BATTERY:
            Serial.println("Status: NO BATTERY");
            break;
    }
    Serial.println("========================");
}
