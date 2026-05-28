#include "weather_layout.h"

static const char* WEEKDAY_NAMES[] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

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
        displayManager.setTextSize(2);
        displayManager.setTextColor(COLOR_BLACK);
        displayManager.setCursor(30, 70);
        displayManager.print("Weather Unavailable");
        displayManager.setTextSize(1);
        displayManager.setCursor(30, 120);
        displayManager.print(reason);
    } while (displayManager.nextPage());
}

void WeatherLayout::draw(WeatherForecastData* weatherData,
                         const struct tm* timeInfo,
                         int batteryPercent, uint16_t batteryVoltage,
                         int sleepMinutes) {
    Serial.println("[UI] Weather draw begin");
    displayManager.setFullWindow();
    displayManager.firstPage();

    do {
        displayManager.fillScreen(COLOR_WHITE);

        drawHeader(timeInfo);
        drawCurrentCondition(weatherData);
        drawHourlyBlocks(weatherData);
        drawTempChart(weatherData);
        drawBottomBar(weatherData, batteryPercent, batteryVoltage, sleepMinutes);

    } while (displayManager.nextPage());
    Serial.println("[UI] Weather draw end");
}

// ─────────────────────────────────────────────
// Header: date + weekday
// ─────────────────────────────────────────────
void WeatherLayout::drawHeader(const struct tm* timeInfo) {
    int y0 = 6;
    int h = 28;

    displayManager.fillRect(6, y0, _screenW - 12, h, COLOR_BLACK);
    displayManager.setTextColor(COLOR_WHITE);
    displayManager.setTextSize(2);

    char title[16];
    snprintf(title, sizeof(title), "Weather");
    displayManager.setCursor(16, y0 + 8);
    displayManager.print(title);

    char dateStr[24];
    int weekday = (timeInfo && timeInfo->tm_wday >= 0 && timeInfo->tm_wday < 7)
        ? timeInfo->tm_wday
        : 0;
    snprintf(dateStr, sizeof(dateStr), "%d-%02d-%02d %s",
             timeInfo->tm_year + 1900, timeInfo->tm_mon + 1, timeInfo->tm_mday,
             WEEKDAY_NAMES[weekday]);
    displayManager.setTextSize(1);
    int16_t x1, y1;
    uint16_t w, h2;
    displayManager.getTextBounds(dateStr, 0, 0, &x1, &y1, &w, &h2);
    displayManager.setCursor(_screenW - 12 - 10 - w, y0 + 10);
    displayManager.print(dateStr);
}

// ─────────────────────────────────────────────
// Current condition: big temp + weather text
// ─────────────────────────────────────────────
void WeatherLayout::drawCurrentCondition(const WeatherForecastData* data) {
    int y0 = 38;
    int h = 30;

    displayManager.setTextColor(COLOR_BLACK);

    if (data && data->count > 0) {
        const HourlyWeatherData& now = data->hours[0];
        displayManager.setTextSize(3);
        displayManager.setCursor(16, y0 + 2);
        displayManager.print(weatherGetIconChar(now.icon).c_str());
        displayManager.print(" ");

        char tempStr[16];
        snprintf(tempStr, sizeof(tempStr), "%sC", now.temp.c_str());
        displayManager.print(tempStr);

        displayManager.setTextSize(1);
        displayManager.setCursor(200, y0 + 8);
        displayManager.print("Icon ");
        displayManager.print(now.icon.c_str());
        displayManager.setCursor(200, y0 + 22);
        displayManager.print("Wind ");
        displayManager.print(now.windScale.c_str());
    } else {
        displayManager.setTextSize(2);
        displayManager.setCursor(16, y0 + 6);
        displayManager.print("--C");
        displayManager.setTextSize(1);
        displayManager.setCursor(200, y0 + 14);
        displayManager.print("No data");
    }
}

// ─────────────────────────────────────────────
// 6-hour blocks: time | temp | icon
// ─────────────────────────────────────────────
void WeatherLayout::drawHourlyBlocks(const WeatherForecastData* data) {
    int y0 = 72;
    int blockH = 56;
    int margin = 6;
    int usableW = _screenW - 2 * margin;
    int colW = usableW / WEATHER_DISPLAY_HOURS;

    displayManager.drawLine(margin, y0 + blockH, _screenW - margin, y0 + blockH, COLOR_BLACK);

    int count = (data && data->count > 0) ? data->count : 0;
    int displayCount = (count > WEATHER_DISPLAY_HOURS) ? WEATHER_DISPLAY_HOURS : count;

    for (int i = 0; i < displayCount; i++) {
        int cx = margin + i * colW + colW / 2;
        const HourlyWeatherData& h = data->hours[i];

        // 解析小时
        String timeStr = "??:??";
        int tIdx = h.fxTime.indexOf('T');
        if (tIdx >= 0) {
            timeStr = h.fxTime.substring(tIdx + 1, tIdx + 6);
        }

        // 时间
        displayManager.setTextSize(1);
        displayManager.setTextColor(COLOR_BLACK);
        int16_t x1, y1;
        uint16_t tw, th;
        displayManager.getTextBounds(timeStr.c_str(), 0, 0, &x1, &y1, &tw, &th);
        displayManager.setCursor(cx - tw / 2, y0 + 10);
        displayManager.print(timeStr.c_str());

        // 温度
        char tempStr[8];
        snprintf(tempStr, sizeof(tempStr), "%sC", h.temp.c_str());
        displayManager.setTextSize(2);
        displayManager.getTextBounds(tempStr, 0, 0, &x1, &y1, &tw, &th);
        displayManager.setCursor(cx - tw / 2, y0 + 28);
        displayManager.print(tempStr);

        // 天气图标
        displayManager.setTextSize(1);
        const String iconChar = weatherGetIconChar(h.icon);
        displayManager.getTextBounds(iconChar.c_str(), 0, 0, &x1, &y1, &tw, &th);
        displayManager.setCursor(cx - tw / 2, y0 + 48);
        displayManager.print(iconChar.c_str());
    }

    // 如果数据不够6个，剩余块画虚线边框
    for (int i = displayCount; i < WEATHER_DISPLAY_HOURS; i++) {
        int x = margin + i * colW;
        displayManager.drawLine(x, y0, x, y0 + blockH, COLOR_BLACK);
    }

    // 分隔竖线
    for (int i = 1; i < WEATHER_DISPLAY_HOURS; i++) {
        int x = margin + i * colW;
        displayManager.drawLine(x, y0, x, y0 + blockH, COLOR_BLACK);
    }
}

// ─────────────────────────────────────────────
// Temperature curve chart
// ─────────────────────────────────────────────
void WeatherLayout::drawTempChart(const WeatherForecastData* data) {
    int chartY0 = 132;
    int chartH = 106;
    int margin = 6;
    int usableW = _screenW - 2 * margin;
    int colW = usableW / WEATHER_DISPLAY_HOURS;

    int count = (data && data->count > 0) ? data->count : 0;
    int displayCount = (count > WEATHER_DISPLAY_HOURS) ? WEATHER_DISPLAY_HOURS : count;

    // 图表边框
    int chartX0 = margin + 28;
    int chartW = usableW - 28;
    displayManager.drawRect(chartX0, chartY0, chartW, chartH, COLOR_BLACK);

    if (displayCount < 2) {
        displayManager.setTextSize(1);
        displayManager.setTextColor(COLOR_BLACK);
        displayManager.setCursor(chartX0 + 20, chartY0 + chartH / 2 - 5);
        displayManager.print("Insufficient data for chart");
        return;
    }

    // 计算温度范围
    int minTemp = 99;
    int maxTemp = -99;
    for (int i = 0; i < displayCount; i++) {
        int t = data->hours[i].temp.toInt();
        if (t < minTemp) minTemp = t;
        if (t > maxTemp) maxTemp = t;
    }
    if (minTemp == maxTemp) {
        minTemp -= 1;
        maxTemp += 1;
    }

    int range = maxTemp - minTemp;
    if (range < 4) {
        int pad = (4 - range) / 2;
        minTemp -= pad;
        maxTemp += pad;
        range = maxTemp - minTemp;
    }

    // Y轴标签（最高/最低温度）
    displayManager.setTextSize(1);
    displayManager.setTextColor(COLOR_BLACK);

    char label[8];
    snprintf(label, sizeof(label), "%dC", maxTemp);
    displayManager.setCursor(margin + 2, chartY0 + 2);
    displayManager.print(label);

    snprintf(label, sizeof(label), "%dC", minTemp);
    displayManager.setCursor(margin + 2, chartY0 + chartH - 10);
    displayManager.print(label);

    // 计算每个数据点的位置
    int dataSpacing = chartW / (displayCount - 1);
    int px[WEATHER_DISPLAY_HOURS];
    int py[WEATHER_DISPLAY_HOURS];

    for (int i = 0; i < displayCount; i++) {
        int t = data->hours[i].temp.toInt();
        px[i] = chartX0 + i * dataSpacing;
        py[i] = chartY0 + chartH - 6 - (int)((float)(t - minTemp) / range * (chartH - 12));
    }

    // 绘制温度曲线（连接线）
    for (int i = 0; i < displayCount - 1; i++) {
        displayManager.drawLine(px[i], py[i], px[i + 1], py[i + 1], COLOR_BLACK);
    }

    // 绘制温度数据点（圆点）
    for (int i = 0; i < displayCount; i++) {
        displayManager.fillCircle(px[i], py[i], 3, COLOR_BLACK);
        displayManager.drawCircle(px[i], py[i], 3, COLOR_WHITE);
        displayManager.fillCircle(px[i], py[i], 2, COLOR_BLACK);
    }

    // 在数据点上方标注温度值
    for (int i = 0; i < displayCount; i++) {
        char val[8];
        snprintf(val, sizeof(val), "%sC", data->hours[i].temp.c_str());
        displayManager.setTextSize(1);
        displayManager.setTextColor(COLOR_BLACK);
        int16_t x1, y1;
        uint16_t tw, th;
        displayManager.getTextBounds(val, 0, 0, &x1, &y1, &tw, &th);
        displayManager.setCursor(px[i] - tw / 2, py[i] - 14);
        displayManager.print(val);
    }

    // 水平参考线（虚线）
    int refY = chartY0 + chartH / 2;
    for (int x = chartX0 + 4; x < chartX0 + chartW - 4; x += 6) {
        displayManager.drawPixel(x, refY, COLOR_BLACK);
    }

    // X轴时间标签（在图表底部）
    for (int i = 0; i < displayCount; i++) {
        String timeStr = "??:??";
        int tIdx = data->hours[i].fxTime.indexOf('T');
        if (tIdx >= 0) {
            timeStr = data->hours[i].fxTime.substring(tIdx + 1, tIdx + 6);
        }
        displayManager.setTextSize(1);
        displayManager.setTextColor(COLOR_BLACK);
        int16_t x1, y1;
        uint16_t tw, th;
        displayManager.getTextBounds(timeStr.c_str(), 0, 0, &x1, &y1, &tw, &th);
        displayManager.setCursor(px[i] - tw / 2, chartY0 + chartH + 2);
        displayManager.print(timeStr.c_str());
    }

}

// ─────────────────────────────────────────────
// Bottom bar: details + battery
// ─────────────────────────────────────────────
void WeatherLayout::drawBottomBar(const WeatherForecastData* data,
                                   int batteryPercent, uint16_t batteryVoltage,
                                   int sleepMinutes) {
    int y0 = 248;
    int h = _screenH - y0 - 6;

    displayManager.fillRect(6, y0, _screenW - 12, h, COLOR_RED);

    displayManager.setTextColor(COLOR_WHITE);
    displayManager.setTextSize(1);

    String details;
    if (data && data->count > 0) {
        const HourlyWeatherData& now = data->hours[0];
        char buf[64];
        snprintf(buf, sizeof(buf), "Wind scale: %s  Hum: %s%%  Pop: %s%%",
                 now.windScale.c_str(),
                 now.humidity.c_str(), now.pop.c_str());
        details = buf;
    } else {
        details = "Weather data unavailable";
    }

    displayManager.setCursor(16, y0 + 6);
    displayManager.print(details.c_str());

    char bottom[48];
    snprintf(bottom, sizeof(bottom), "Update every %dmin  Bat: %d%% %dmV",
             sleepMinutes, batteryPercent, batteryVoltage);
    displayManager.setCursor(16, y0 + 20);
    displayManager.print(bottom);
}
