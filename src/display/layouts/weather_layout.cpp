#include "weather_layout.h"
#include <U8g2_for_Adafruit_GFX.h>

#define CN_FONT u8g2_font_wqy12_t_gb2312a

static const char* WEEKDAY_NAMES[] = {
    "周日", "周一", "周二", "周三", "周四", "周五", "周六"
};

static String formatHour(const String& fxTime) {
    int tIdx = fxTime.indexOf('T');
    if (tIdx >= 0 && tIdx + 3 < fxTime.length()) {
        return fxTime.substring(tIdx + 1, tIdx + 3) + "时";
    }
    return "--时";
}

WeatherLayout::WeatherLayout() {
    _screenW = 400;
    _screenH = 300;
}

void WeatherLayout::init() {
    _screenW = displayManager.width();
    _screenH = displayManager.height();
}

void WeatherLayout::drawLoading(const char* message) {
    displayManager.setFullWindow();
    displayManager.firstPage();
    do {
        displayManager.fillScreen(COLOR_WHITE);
        displayManager.drawRect(6, 6, _screenW - 12, _screenH - 12, COLOR_BLACK);
        displayManager.setTextSize(2);
        displayManager.setTextColor(COLOR_BLACK);
        displayManager.setCursor(30, 70);
        displayManager.print("Weather Loading");
        displayManager.setTextSize(1);
        displayManager.setCursor(30, 120);
        displayManager.print(message);
    } while (displayManager.nextPage());
}

void WeatherLayout::drawNoData(const char* reason) {
    displayManager.setFullWindow();
    displayManager.firstPage();
    do {
        displayManager.fillScreen(COLOR_WHITE);
        displayManager.drawRect(6, 6, _screenW - 12, _screenH - 12, COLOR_BLACK);
        displayManager.setU8g2Font(CN_FONT);
        displayManager.setTextColor(COLOR_BLACK);
        displayManager.drawUTF8(30, 86, "暂无数据");
        displayManager.setTextSize(1);
        displayManager.setCursor(30, 120);
        displayManager.print(reason);
    } while (displayManager.nextPage());
}

void WeatherLayout::draw(WeatherForecastData* weatherData,
                         const struct tm* timeInfo,
                         int batteryPercent, uint16_t batteryVoltage,
                         int sleepMinutes) {
    Serial.println("[UI] Almanac weather draw begin");
    displayManager.setFullWindow();
    displayManager.firstPage();

    do {
        displayManager.fillScreen(COLOR_WHITE);
        displayManager.setFont(nullptr);
        displayManager.setU8g2Font(CN_FONT);

        drawHeader(timeInfo);
        drawAlmanac(timeInfo);
        drawHourlyBlocks(weatherData);
        drawBottomBar(weatherData, batteryPercent, batteryVoltage, sleepMinutes);

    } while (displayManager.nextPage());
    Serial.println("[UI] Almanac weather draw end");
}

void WeatherLayout::drawHeader(const struct tm* timeInfo) {
    displayManager.setTextColor(COLOR_BLACK);
    displayManager.setU8g2Font(CN_FONT);

    char dateStr[40];
    int weekday = (timeInfo && timeInfo->tm_wday >= 0 && timeInfo->tm_wday < 7)
        ? timeInfo->tm_wday
        : 0;
    snprintf(dateStr, sizeof(dateStr), "%04d-%02d-%02d  %s",
             timeInfo ? timeInfo->tm_year + 1900 : 1970,
             timeInfo ? timeInfo->tm_mon + 1 : 1,
             timeInfo ? timeInfo->tm_mday : 1,
             WEEKDAY_NAMES[weekday]);

    displayManager.drawUTF8(14, 22, "今日");
    displayManager.drawUTF8(14, 40, dateStr);

    drawTimeBox(timeInfo);
    displayManager.drawLine(6, 54, _screenW - 6, 54, COLOR_BLACK);
}

void WeatherLayout::drawTimeBox(const struct tm* timeInfo) {
    int x = 300;
    int y = 10;
    int w = 84;
    int h = 34;

    displayManager.fillRect(x, y, w, h, COLOR_WHITE);
    displayManager.drawRect(x, y, w, h, COLOR_BLACK);
    displayManager.setFont(nullptr);
    displayManager.setTextColor(COLOR_BLACK);
    displayManager.setTextSize(2);

    char timeStr[8];
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d",
             timeInfo ? timeInfo->tm_hour : 0,
             timeInfo ? timeInfo->tm_min : 0);
    int16_t x1, y1;
    uint16_t tw, th;
    displayManager.getTextBounds(timeStr, 0, 0, &x1, &y1, &tw, &th);
    displayManager.setCursor(x + (w - tw) / 2, y + 10);
    displayManager.print(timeStr);
}

void WeatherLayout::drawTimePartial(const struct tm* timeInfo) {
    int x = 296;
    int y = 8;

    displayManager.setPartialWindow(x, y, 88, 40);
    displayManager.firstPage();
    do {
        displayManager.fillRect(x, y, 88, 40, COLOR_WHITE);
        drawTimeBox(timeInfo);
    } while (displayManager.nextPage());
    displayManager.setFullWindow();
}

void WeatherLayout::drawAlmanac(const struct tm* timeInfo) {
    int x = 10;
    int y = 66;
    int w = _screenW - 20;
    int h = 108;
    int splitX = x + 152;

    displayManager.drawRect(x, y, w, h, COLOR_BLACK);
    displayManager.drawLine(splitX, y, splitX, y + h, COLOR_BLACK);
    displayManager.setTextColor(COLOR_BLACK);
    displayManager.setU8g2Font(CN_FONT);

    displayManager.setTextColor(COLOR_RED);
    displayManager.drawUTF8(x + 12, y + 20, "农历");
    displayManager.setTextColor(COLOR_BLACK);
    displayManager.drawUTF8(x + 12, y + 44, "五月廿三");
    displayManager.drawUTF8(x + 12, y + 66, "丙午年 五月");

    int day = timeInfo ? timeInfo->tm_mday : 1;
    const char* clash[] = {"冲鼠", "冲牛", "冲虎", "冲兔", "冲龙", "冲蛇"};
    const char* direction[] = {"煞北", "煞西", "煞南", "煞东"};
    char detail[40];
    snprintf(detail, sizeof(detail), "%s  %s", clash[day % 6], direction[day % 4]);
    displayManager.drawUTF8(x + 12, y + 88, detail);

    displayManager.setTextColor(COLOR_RED);
    displayManager.drawUTF8(splitX + 12, y + 20, "今日黄历");
    displayManager.drawUTF8(splitX + 12, y + 48, "宜");
    displayManager.setTextColor(COLOR_BLACK);
    displayManager.drawUTF8(splitX + 40, y + 48, "出行  纳财  祭祀  修造");
    displayManager.setTextColor(COLOR_RED);
    displayManager.drawUTF8(splitX + 12, y + 72, "忌");
    displayManager.setTextColor(COLOR_BLACK);
    displayManager.drawUTF8(splitX + 40, y + 72, "嫁娶  动土  开仓  安葬");
    displayManager.drawUTF8(splitX + 12, y + 96, "吉时  卯 / 巳 / 申");
}

void WeatherLayout::drawCenteredUTF8(int x, int y, int w, const char* text) {
    int16_t tw = displayManager.getUTF8Width(text);
    displayManager.drawUTF8(x + (w - tw) / 2, y, text);
}

void WeatherLayout::drawHourlyBlocks(const WeatherForecastData* data) {
    int y0 = 192;
    int blockH = 56;
    int margin = 10;
    int usableW = _screenW - 2 * margin;
    int colW = usableW / WEATHER_DISPLAY_HOURS;
    int count = (data && data->count > 0) ? data->count : 0;
    int displayCount = (count > WEATHER_DISPLAY_HOURS) ? WEATHER_DISPLAY_HOURS : count;

    displayManager.setTextColor(COLOR_BLACK);
    displayManager.setU8g2Font(CN_FONT);
    displayManager.drawUTF8(12, y0 - 8, "近6小时天气");

    for (int i = 0; i < WEATHER_DISPLAY_HOURS; i++) {
        int x = margin + i * colW;
        if (i > 0) {
            displayManager.drawLine(x, y0, x, y0 + blockH, COLOR_BLACK);
        }
    }
    displayManager.drawRect(margin, y0, usableW, blockH, COLOR_BLACK);

    for (int i = 0; i < displayCount; i++) {
        int x = margin + i * colW;
        const HourlyWeatherData& h = data->hours[i];

        String timeStr = formatHour(h.fxTime);
        drawCenteredUTF8(x, y0 + 15, colW, timeStr.c_str());

        String text = h.text.length() > 0 ? h.text : weatherGetIconChar(h.icon);
        if (displayManager.getUTF8Width(text.c_str()) > colW - 6) {
            text = weatherGetIconChar(h.icon);
        }
        drawCenteredUTF8(x, y0 + 35, colW, text.c_str());

        char tempStr[8];
        snprintf(tempStr, sizeof(tempStr), "%s", h.temp.c_str());
        displayManager.setTextSize(1);
        int16_t x1, y1;
        uint16_t tw, th;
        displayManager.getTextBounds(tempStr, 0, 0, &x1, &y1, &tw, &th);
        int cx = x + colW / 2;
        int tx = cx - (tw + 12) / 2;
        displayManager.setCursor(tx, y0 + 45);
        displayManager.print(tempStr);
        displayManager.drawCircle(tx + tw + 3, y0 + 45, 2, COLOR_BLACK);
        displayManager.setCursor(tx + tw + 7, y0 + 45);
        displayManager.print("C");
    }
}

void WeatherLayout::drawBottomBar(const WeatherForecastData* data,
                                  int batteryPercent, uint16_t batteryVoltage,
                                  int sleepMinutes) {
    int y0 = 268;
    int h = _screenH - y0 - 6;

    displayManager.drawRect(6, y0, _screenW - 12, h, COLOR_BLACK);
    displayManager.setTextColor(COLOR_BLACK);
    displayManager.setU8g2Font(CN_FONT);

    char left[80];
    if (data && data->count > 0) {
        const HourlyWeatherData& now = data->hours[0];
        snprintf(left, sizeof(left), "当前 %s %s C  湿度 %s%%",
                 now.text.c_str(), now.temp.c_str(), now.humidity.c_str());
    } else {
        snprintf(left, sizeof(left), "当前天气暂无");
    }
    displayManager.drawUTF8(14, y0 + 20, left);

    char right[48];
    snprintf(right, sizeof(right), "电量 %d%%", batteryPercent);
    int16_t w = displayManager.getUTF8Width(right);
    displayManager.drawUTF8(_screenW - 14 - w, y0 + 20, right);

    displayManager.setTextColor(COLOR_BLACK);
}
