/**
 * @file epaper_driver.cpp
 * @brief E-Paper display driver implementation
 */

#include "epaper_driver.h"

#include <SPI.h>

namespace {
constexpr uint16_t EPD_RESET_DURATION_MS = 10;
}

// ============================================
// Display Instance Definition
// ============================================
#if SCREEN_COLORS == 3
    GxEPD2_3C<EPD_DRIVER, EPD_DRIVER::HEIGHT> display(
        EPD_DRIVER(SPI_CS, SPI_DC, SPI_RST, SPI_BUSY)
    );
#else
    GxEPD2_BW<EPD_DRIVER, EPD_DRIVER::HEIGHT> display(
        EPD_DRIVER(SPI_CS, SPI_DC, SPI_RST, SPI_BUSY)
    );
#endif

void epaperInit(uint32_t serial_diag_bitrate) {
    // 初始化参数说明：
    // - serial_diag_bitrate: 串口诊断波特率
    // - initial: 是否首次初始化
    // - reset_duration: 复位脉冲时长(ms)，10ms 与 jcalendar/Z21 测试固件一致
    // - pulldown_rst_mode: RST下拉模式
    SPI.begin(SPI_SCK, -1, SPI_MOSI, SPI_CS);
    display.init(serial_diag_bitrate, true, EPD_RESET_DURATION_MS, false);
    
    // 设置默认旋转方向
    display.setRotation(0);
    
    Serial.println("[EPD] Display initialized");
    Serial.printf("[EPD] Resolution: %dx%d\n", SCREEN_WIDTH, SCREEN_HEIGHT);
    Serial.printf("[EPD] Colors: %d\n", SCREEN_COLORS);
    Serial.printf("[EPD] Partial update: %s\n", HAS_PARTIAL ? "Yes" : "No");
}

void epaperReinit(bool initial) {
    SPI.begin(SPI_SCK, -1, SPI_MOSI, SPI_CS);
    display.init(0, initial, EPD_RESET_DURATION_MS, false);
    display.setRotation(0);
}

void epaperPowerOff(void) {
    display.powerOff();
    Serial.println("[EPD] Power off");
}

void epaperHibernate(void) {
    display.hibernate();
    Serial.println("[EPD] Hibernate");
}

bool epaperHasPartialUpdate(void) {
    return HAS_PARTIAL;
}

bool epaperHasFastPartialUpdate(void) {
    return HAS_FAST_PARTIAL;
}

int16_t epaperWidth(void) {
    return display.width();
}

int16_t epaperHeight(void) {
    return display.height();
}
