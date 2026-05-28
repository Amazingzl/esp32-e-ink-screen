#include <Arduino.h>
#include <GxEPD2_3C.h>
#include <U8g2_for_Adafruit_GFX.h>

// ============================================
// Pin Definitions - ESP32 C3
// 驱动板标注 SDA -> GPIO5 (MOSI)
// ============================================
#ifndef SPI_MOSI
#define SPI_MOSI  GPIO_NUM_5   // SDA/MOSI
#endif
#ifndef SPI_SCK
#define SPI_SCK   GPIO_NUM_4   // SCK/SCL
#endif
#ifndef SPI_CS
#define SPI_CS    GPIO_NUM_6   // CS
#endif
#ifndef SPI_DC
#define SPI_DC    GPIO_NUM_7   // DC
#endif
#ifndef SPI_RST
#define SPI_RST   GPIO_NUM_8   // RST
#endif
#ifndef SPI_BUSY
#define SPI_BUSY  GPIO_NUM_3   // BUSY
#endif

GxEPD2_3C<GxEPD2_420c_Z21, GxEPD2_420c_Z21::HEIGHT> display(
    GxEPD2_420c_Z21(SPI_CS, SPI_DC, SPI_RST, SPI_BUSY)
);

U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;

bool diagnoseBusy() {
    // 诊断 BUSY 引脚在不同模式下的电平
    pinMode(SPI_BUSY, INPUT);
    delay(1);
    int val_input = digitalRead(SPI_BUSY);
    pinMode(SPI_BUSY, INPUT_PULLUP);
    delay(1);
    int val_pullup = digitalRead(SPI_BUSY);
    pinMode(SPI_BUSY, INPUT_PULLDOWN);
    delay(1);
    int val_pulldown = digitalRead(SPI_BUSY);
    pinMode(SPI_BUSY, INPUT);
    delay(1);

    Serial.println("--- BUSY Pin Diagnosis ---");
    Serial.printf("  INPUT mode:        %d\n", val_input);
    Serial.printf("  INPUT_PULLUP mode:  %d\n", val_pullup);
    Serial.printf("  INPUT_PULLDOWN mode:%d\n", val_pulldown);
    if (val_pullup == 1 && val_pulldown == 0) {
        Serial.println("  WARN: BUSY follows internal pulls; this pin is floating or not connected.");
    } else if (val_input == val_pullup && val_input == val_pulldown) {
        Serial.println("  OK: BUSY is held by the panel/board or an external pull.");
    } else {
        Serial.println("  WARN: BUSY level is ambiguous; verify the wire and panel power.");
    }
    Serial.println("----------------------------\n");
    return val_pullup == 1 && val_pulldown == 0;
}

void setup() {
    Serial.begin(115200);
    delay(100);

    Serial.println("\n=================================");
    Serial.println("  E-Paper Display Test v3");
    Serial.println("=================================\n");

    Serial.println("Pin Configuration:");
    Serial.printf("  SDA/MOSI: GPIO %d\n", SPI_MOSI);
    Serial.printf("  SCK:      GPIO %d\n", SPI_SCK);
    Serial.printf("  CS:       GPIO %d\n", SPI_CS);
    Serial.printf("  DC:       GPIO %d\n", SPI_DC);
    Serial.printf("  RST:      GPIO %d\n", SPI_RST);
    Serial.printf("  BUSY:     GPIO %d\n", SPI_BUSY);
    Serial.println();

    // 1. 在 SPI 初始化前诊断 BUSY
    Serial.println("=== Diagnosis BEFORE SPI init ===");
    bool busyFloating = diagnoseBusy();

    // 2. 初始化 SPI
    Serial.println("Initializing SPI...");
    SPI.begin(SPI_SCK, -1, SPI_MOSI, SPI_CS);

    // 3. 诊断 RST 操作
    Serial.println("Testing RST pin...");
    pinMode(SPI_RST, OUTPUT);
    digitalWrite(SPI_RST, LOW);
    delay(10);
    digitalWrite(SPI_RST, HIGH);
    delay(10);
    // 复位后诊断 BUSY (看屏幕是否响应复位)
    Serial.println("=== Diagnosis AFTER RST ===");
    busyFloating = diagnoseBusy() || busyFloating;

    // 4. 初始化显示屏
    Serial.println("Initializing display...");
    display.init(115200, true, 10, false);
    display.setRotation(0);

    // 5. init() 后诊断 BUSY
    Serial.println("=== Diagnosis AFTER init ===");
    busyFloating = diagnoseBusy() || busyFloating;
    if (busyFloating) {
        Serial.println("!!! BUSY looks floating. A 4.2 inch full refresh should not finish in 1ms.");
        Serial.println("!!! Check BUSY wiring first, or try env:test-epaper-busy9 if BUSY is wired to GPIO9.\n");
    }

    Serial.printf("Resolution: %dx%d\n", display.width(), display.height());
    Serial.println("Display initialized!\n");
    delay(500);

    // Test 1: 全白清屏
    Serial.println("Test 1: Full white clear...");
    Serial.printf("  BUSY before refresh: %d\n", digitalRead(SPI_BUSY));
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
    } while (display.nextPage());
    Serial.printf("  BUSY after refresh: %d\n", digitalRead(SPI_BUSY));
    Serial.println("Test 1 done");
    delay(2000);

    // Test 2: 绘制色块
    Serial.println("Test 2: Color blocks...");
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        display.fillRect(10, 10, 180, 130, GxEPD_BLACK);
        display.fillRect(210, 10, 180, 130, GxEPD_RED);
        display.fillRect(10, 160, 380, 130, GxEPD_BLACK);
        display.fillRect(30, 180, 100, 80, GxEPD_RED);
        display.fillRect(150, 180, 100, 80, GxEPD_WHITE);
        display.fillRect(270, 180, 100, 80, GxEPD_WHITE);
        display.drawRect(5, 5, 390, 290, GxEPD_BLACK);
    } while (display.nextPage());
    Serial.println("Test 2 done");
    delay(2000);

    // Test 3: 文字显示
    Serial.println("Test 3: Text display...");
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);

        u8g2Fonts.begin(display);
        u8g2Fonts.setFontMode(1);
        u8g2Fonts.setBackgroundColor(GxEPD_WHITE);

        u8g2Fonts.setFont(u8g2_font_wqy16_t_gb2312);
        u8g2Fonts.setForegroundColor(GxEPD_BLACK);
        u8g2Fonts.setCursor(50, 50);
        u8g2Fonts.print("墨水屏测试成功!");

        u8g2Fonts.setForegroundColor(GxEPD_RED);
        u8g2Fonts.setCursor(30, 100);
        u8g2Fonts.print("E-Paper Display OK");

        u8g2Fonts.setForegroundColor(GxEPD_BLACK);
        u8g2Fonts.setFont(u8g2_font_wqy12_t_gb2312);
        u8g2Fonts.setCursor(30, 150);
        u8g2Fonts.print("型号: GDEQ042Z21");
        u8g2Fonts.setCursor(30, 180);
        u8g2Fonts.print("尺寸: 4.2寸 三色屏");
        u8g2Fonts.setCursor(30, 210);
        u8g2Fonts.print("分辨率: 400x300");

        u8g2Fonts.setFont(u8g2_font_helvB12_tf);
        u8g2Fonts.setForegroundColor(GxEPD_RED);
        u8g2Fonts.setCursor(100, 270);
        u8g2Fonts.print("Test Passed!");

        display.drawRect(20, 120, 360, 110, GxEPD_BLACK);
    } while (display.nextPage());
    Serial.println("Test 3 done");

    Serial.println("\nAll tests completed!");
}

void loop() {
    delay(1000);
}
