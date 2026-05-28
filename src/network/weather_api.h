#ifndef __WEATHER_API_H__
#define __WEATHER_API_H__

#include <Arduino.h>

// ============================================
// QWeather API Configuration
// 请在这里填入你的和风天气 API 信息和城市 LocationID
// ============================================
#define WEATHER_API_KEY         "4894c899bec548e5b189ea8829efb1ef"
#define WEATHER_LOCATION_ID     "101230112"
#define WEATHER_API_HOST        "ne3aarcept.re.qweatherapi.com"

// 每小时预报数据结构
typedef struct {
    String  fxTime;         // 预报时间 (ISO 8601)
    String  temp;           // 温度 (°C)
    String  icon;           // 天气图标代码
    String  text;           // 天气描述文字
    String  windDir;        // 风向
    String  windScale;      // 风力等级
    String  humidity;       // 相对湿度 (%)
    String  pop;            // 降水概率 (%)
    String  precip;         // 降水量 (mm)
    String  pressure;       // 气压 (hPa)
} HourlyWeatherData;

// 天气数据容器
typedef struct {
    String          updateTime;     // 数据更新时间
    String          locationName;   // 地点名称（需额外获取）
    int             count;          // 有效数据条数
    HourlyWeatherData hours[24];    // 最多24条小时预报
} WeatherForecastData;

bool weatherInit(void);
bool weatherFetchHourly(WeatherForecastData* data, int hours = 12);
void weatherPrintData(const WeatherForecastData* data);
String weatherGetIconChar(const String& iconCode);

#endif