/**
 * @file display_manager.cpp
 * @brief Display manager implementation
 */

#include "display_manager.h"
#include <U8g2_for_Adafruit_GFX.h>

// 全局实例
DisplayManager displayManager;
static U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;

DisplayManager::DisplayManager() 
    : _initialized(false)
    , _inPartialMode(false) 
{
}

void DisplayManager::init() {
    epaperInit(115200);
    u8g2Fonts.begin(display);
    u8g2Fonts.setFontMode(1);
    u8g2Fonts.setFontDirection(0);
    u8g2Fonts.setForegroundColor(COLOR_BLACK);
    u8g2Fonts.setBackgroundColor(COLOR_WHITE);
    _initialized = true;
    _inPartialMode = false;
}

// ========================================
// Window Management
// ========================================

void DisplayManager::setFullWindow() {
    display.setFullWindow();
    _inPartialMode = false;
}

void DisplayManager::setPartialWindow(int16_t x, int16_t y, int16_t w, int16_t h) {
    // 对齐到8的倍数
    int16_t alignedX = alignTo8(x);
    int16_t alignedW = alignTo8(w + (x - alignedX));
    
    // 边界检查
    if (alignedX < 0) alignedX = 0;
    if (alignedW < 8) alignedW = 8;
    if (alignedX + alignedW > display.width()) {
        alignedW = display.width() - alignedX;
    }
    
    display.setPartialWindow(alignedX, y, alignedW, h);
    _inPartialMode = true;
}

bool DisplayManager::isPartialMode() {
    return _inPartialMode;
}

// ========================================
// Drawing Operations
// ========================================

void DisplayManager::firstPage() {
    display.firstPage();
}

bool DisplayManager::nextPage() {
    return display.nextPage();
}

void DisplayManager::fillScreen(uint16_t color) {
    display.fillScreen(color);
}

void DisplayManager::drawPixel(int16_t x, int16_t y, uint16_t color) {
    display.drawPixel(x, y, color);
}

void DisplayManager::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
    display.drawLine(x0, y0, x1, y1, color);
}

void DisplayManager::drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    display.drawRect(x, y, w, h, color);
}

void DisplayManager::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    display.fillRect(x, y, w, h, color);
}

void DisplayManager::drawCircle(int16_t x, int16_t y, int16_t r, uint16_t color) {
    display.drawCircle(x, y, r, color);
}

void DisplayManager::fillCircle(int16_t x, int16_t y, int16_t r, uint16_t color) {
    display.fillCircle(x, y, r, color);
}

void DisplayManager::drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, 
                                  int16_t x2, int16_t y2, uint16_t color) {
    display.drawTriangle(x0, y0, x1, y1, x2, y2, color);
}

// ========================================
// Text Operations
// ========================================

void DisplayManager::setFont(const GFXfont* font) {
    display.setFont(font);
}

void DisplayManager::setU8g2Font(const uint8_t* font) {
    u8g2Fonts.setFont(font);
}

void DisplayManager::setTextColor(uint16_t color) {
    display.setTextColor(color);
    u8g2Fonts.setForegroundColor(color);
}

void DisplayManager::setTextColor(uint16_t color, uint16_t background) {
    display.setTextColor(color, background);
    u8g2Fonts.setForegroundColor(color);
    u8g2Fonts.setBackgroundColor(background);
}

void DisplayManager::setTextSize(uint8_t size) {
    display.setTextSize(size);
}

void DisplayManager::setCursor(int16_t x, int16_t y) {
    display.setCursor(x, y);
}

void DisplayManager::print(const char* text) {
    display.print(text);
}

void DisplayManager::print(const String& text) {
    display.print(text);
}

void DisplayManager::print(int num) {
    display.print(num);
}

void DisplayManager::print(float num, int digits) {
    display.print(num, digits);
}

void DisplayManager::drawUTF8(int16_t x, int16_t y, const char* text) {
    u8g2Fonts.drawUTF8(x, y, text);
}

int16_t DisplayManager::getUTF8Width(const char* text) {
    return u8g2Fonts.getUTF8Width(text);
}

void DisplayManager::getTextBounds(const char* text, int16_t x, int16_t y,
                                   int16_t* x1, int16_t* y1, uint16_t* w, uint16_t* h) {
    display.getTextBounds(text, x, y, x1, y1, w, h);
}

// ========================================
// Refresh Operations
// ========================================

void DisplayManager::fullRefresh() {
    display.display(false);  // false = full refresh
    _inPartialMode = false;
}

void DisplayManager::partialRefresh() {
    if (HAS_PARTIAL) {
        display.display(true);  // true = partial refresh
    } else {
        fullRefresh();
    }
}

void DisplayManager::refreshArea(int16_t x, int16_t y, int16_t w, int16_t h) {
    setPartialWindow(x, y, w, h);
    partialRefresh();
}

// ========================================
// Power Management
// ========================================

void DisplayManager::powerOff() {
    epaperPowerOff();
}

void DisplayManager::hibernate() {
    epaperHibernate();
}

// ========================================
// Utilities
// ========================================

int16_t DisplayManager::width() {
    return display.width();
}

int16_t DisplayManager::height() {
    return display.height();
}

void DisplayManager::setRotation(uint8_t rotation) {
    display.setRotation(rotation);
}

uint8_t DisplayManager::getRotation() {
    return display.getRotation();
}

int16_t DisplayManager::alignTo8(int16_t value) {
    return (value >> 3) << 3;  // 向下对齐到8的倍数
}
