#ifndef __WEATHER_LAYOUT_H__
#define __WEATHER_LAYOUT_H__

#include <Arduino.h>
#include "../display_manager.h"
#include "../../network/weather_api.h"

// 一次显示的小时数
#define WEATHER_DISPLAY_HOURS   6

class WeatherLayout {
public:
    WeatherLayout();
    void init();
    void draw(WeatherForecastData* weatherData, const struct tm* timeInfo,
              int batteryPercent, uint16_t batteryVoltage, int sleepMinutes);
    void drawLoading(const char* message);
    void drawNoData(const char* reason);
    void drawTimePartial(const struct tm* timeInfo);

private:
    int _screenW;
    int _screenH;

    void drawHeader(const struct tm* timeInfo);
    void drawTimeBox(const struct tm* timeInfo);
    void drawAlmanac(const struct tm* timeInfo);
    void drawHourlyBlocks(const WeatherForecastData* data);
    void drawCenteredUTF8(int x, int y, int w, const char* text);
    void drawBottomBar(const WeatherForecastData* data,
                       int batteryPercent, uint16_t batteryVoltage, int sleepMinutes);
};

#endif
