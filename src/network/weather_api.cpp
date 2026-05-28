#include "weather_api.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <miniz.h>

static constexpr size_t WEATHER_GZIP_MAX_DECOMPRESSED = 8192;
static char g_decompressedWeatherJson[WEATHER_GZIP_MAX_DECOMPRESSED + 1];
static String g_plainWeatherJson;

bool weatherInit(void) {
    if (strcmp(WEATHER_API_KEY, "YOUR_API_KEY_HERE") == 0) {
        Serial.println("[Weather] API Key not configured");
        return false;
    }
    if (strcmp(WEATHER_LOCATION_ID, "YOUR_LOCATION_ID_HERE") == 0) {
        Serial.println("[Weather] Location ID not configured");
        return false;
    }
    return true;
}

static bool parseGzipDeflatePayload(const uint8_t* gzipData, size_t gzipLen,
                                    const uint8_t** deflateData, size_t* deflateLen,
                                    size_t* uncompressedLen) {
    if (!gzipData || !deflateData || !deflateLen || !uncompressedLen || gzipLen < 18) {
        return false;
    }

    if (gzipData[0] != 0x1f || gzipData[1] != 0x8b || gzipData[2] != 8) {
        Serial.println("[Gzip] Invalid gzip header");
        return false;
    }

    const uint8_t flags = gzipData[3];
    size_t offset = 10;

    if (flags & 0x04) {
        if (offset + 2 > gzipLen) return false;
        size_t extraLen = gzipData[offset] | (gzipData[offset + 1] << 8);
        offset += 2 + extraLen;
        if (offset > gzipLen) return false;
    }

    if (flags & 0x08) {
        while (offset < gzipLen && gzipData[offset++] != 0) {}
        if (offset >= gzipLen) return false;
    }

    if (flags & 0x10) {
        while (offset < gzipLen && gzipData[offset++] != 0) {}
        if (offset >= gzipLen) return false;
    }

    if (flags & 0x02) {
        offset += 2;
        if (offset > gzipLen) return false;
    }

    if (offset + 8 > gzipLen) {
        Serial.println("[Gzip] Truncated gzip payload");
        return false;
    }

    *deflateData = gzipData + offset;
    *deflateLen = gzipLen - offset - 8;  // Skip gzip CRC32 and ISIZE trailer.
    *uncompressedLen = static_cast<size_t>(gzipData[gzipLen - 4]) |
                       (static_cast<size_t>(gzipData[gzipLen - 3]) << 8) |
                       (static_cast<size_t>(gzipData[gzipLen - 2]) << 16) |
                       (static_cast<size_t>(gzipData[gzipLen - 1]) << 24);
    return true;
}

static char* gzipDecompressToBuffer(const uint8_t* compressedData, size_t compressedLen,
                                    size_t* outputLen) {
    if (!outputLen) {
        return nullptr;
    }
    *outputLen = 0;

    const uint8_t* deflateData = nullptr;
    size_t deflateLen = 0;
    size_t uncompressedLen = 0;
    if (!parseGzipDeflatePayload(compressedData, compressedLen, &deflateData, &deflateLen,
                                 &uncompressedLen)) {
        return nullptr;
    }

    if (uncompressedLen == 0) {
        Serial.println("[Gzip] Empty gzip payload");
        return nullptr;
    }

    if (uncompressedLen > WEATHER_GZIP_MAX_DECOMPRESSED) {
        Serial.printf("[Gzip] Payload too large, max=%u actual=%u\n",
                      static_cast<unsigned>(WEATHER_GZIP_MAX_DECOMPRESSED),
                      static_cast<unsigned>(uncompressedLen));
        return nullptr;
    }

    memset(g_decompressedWeatherJson, 0, sizeof(g_decompressedWeatherJson));

    size_t actualLen = tinfl_decompress_mem_to_mem(g_decompressedWeatherJson, WEATHER_GZIP_MAX_DECOMPRESSED,
                                                   deflateData, deflateLen, 0);
    if (actualLen == TINFL_DECOMPRESS_MEM_TO_MEM_FAILED) {
        Serial.println("[Gzip] tinfl decompress failed");
        return nullptr;
    }

    if (actualLen != uncompressedLen) {
        Serial.printf("[Gzip] Size mismatch, trailer: %u actual: %u\n",
                      static_cast<unsigned>(uncompressedLen),
                      static_cast<unsigned>(actualLen));
        return nullptr;
    }

    g_decompressedWeatherJson[actualLen] = '\0';
    *outputLen = actualLen;

    Serial.printf("[Gzip] Decompressed: %u -> %u bytes\n",
                  static_cast<unsigned>(compressedLen),
                  static_cast<unsigned>(actualLen));
    return g_decompressedWeatherJson;
}

static const char* findWithin(const char* begin, const char* end, const char* needle) {
    size_t needleLen = strlen(needle);
    if (!begin || !end || begin >= end || needleLen == 0) {
        return nullptr;
    }

    for (const char* p = begin; p + needleLen <= end; p++) {
        if (memcmp(p, needle, needleLen) == 0) {
            return p;
        }
    }
    return nullptr;
}

static bool jsonReadStringField(const char* begin, const char* end,
                                const char* key, char* out, size_t outSize) {
    if (!begin || !end || !key || !out || outSize == 0) {
        return false;
    }

    out[0] = '\0';

    char pattern[32];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);

    const char* p = findWithin(begin, end, pattern);
    if (!p) {
        return false;
    }

    p += strlen(pattern);
    while (p < end && *p != ':') p++;
    if (p >= end) return false;
    p++;
    while (p < end && (*p == ' ' || *p == '\r' || *p == '\n' || *p == '\t')) p++;
    if (p >= end || *p != '"') return false;
    p++;

    size_t n = 0;
    while (p < end && *p != '"') {
        if (*p == '\\' && p + 1 < end) {
            p++;
        }
        if (n + 1 < outSize) {
            out[n++] = *p;
        }
        p++;
    }
    out[n] = '\0';
    return p < end;
}

static bool httpGetJsonPayload(const String& url, const char** jsonData, size_t* jsonLen) {
    if (!jsonData || !jsonLen) {
        return false;
    }
    *jsonData = nullptr;
    *jsonLen = 0;

    HTTPClient http;
    WiFiClientSecure wifiClient;
    wifiClient.setInsecure();  // 跳过证书验证
    
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    
    // 设置需要收集的响应头
    const char* headerKeys[] = {"Content-Encoding"};
    http.collectHeaders(headerKeys, 1);
    
    if (!http.begin(wifiClient, url)) {
        Serial.println("[HTTP] Unable to connect");
        return false;
    }
    
    http.setConnectTimeout(5000);
    http.setTimeout(8000);
    
    // 添加请求头
    http.addHeader("X-QW-Api-Key", WEATHER_API_KEY);
    http.addHeader("Accept-Encoding", "identity");

    int httpCode = http.GET();
    Serial.printf("[HTTP] Code: %d\n", httpCode);
    
    if (httpCode != 200) {
        Serial.printf("[HTTP] GET failed, code: %d\n", httpCode);
        http.end();
        return false;
    }

    // 检查是否是 Gzip 压缩
    bool isGzip = false;
    int headers = http.headers();
    for (int i = 0; i < headers; i++) {
        String headerName = http.headerName(i);
        String headerValue = http.header(i);
        if (headerName.equalsIgnoreCase("Content-Encoding") &&
            headerValue.indexOf("gzip") >= 0) {
            isGzip = true;
            break;
        }
    }
    
    Serial.printf("[HTTP] Gzip: %s\n", isGzip ? "yes" : "no");

    String payload = http.getString();
    http.end();
    
    Serial.printf("[HTTP] Payload length: %d\n", payload.length());

    if (isGzip) {
        size_t decompressedLen = 0;
        char* decompressedPayload = gzipDecompressToBuffer(
            reinterpret_cast<const uint8_t*>(payload.c_str()),
            payload.length(),
            &decompressedLen);
        if (!decompressedPayload) {
            Serial.println("[Gzip] Decompress failed");
            return false;
        }

        *jsonData = decompressedPayload;
        *jsonLen = decompressedLen;
    } else {
        g_plainWeatherJson = payload;
        *jsonData = g_plainWeatherJson.c_str();
        *jsonLen = g_plainWeatherJson.length();
    }

    char code[8];
    if (!jsonReadStringField(*jsonData, *jsonData + *jsonLen, "code", code, sizeof(code))) {
        Serial.println("[Weather] Missing API code");
        return false;
    }

    if (strcmp(code, "200") != 0) {
        Serial.printf("[Weather] API error code: %s\n", code);
        return false;
    }

    return true;
}

bool weatherFetchHourly(WeatherForecastData* data, int hours) {
    if (!data) return false;

    data->count = 0;
    if (hours < 1) hours = 1;
    if (hours > 24) hours = 24;

    char url[256];
    snprintf(url, sizeof(url),
        "https://%s/v7/weather/%dh?location=%s",
        WEATHER_API_HOST,
        24,
        WEATHER_LOCATION_ID);

    Serial.printf("[Weather] URL: %s\n", url);

    const char* jsonData = nullptr;
    size_t jsonLen = 0;
    if (!httpGetJsonPayload(url, &jsonData, &jsonLen)) {
        return false;
    }

    const char* jsonEnd = jsonData + jsonLen;
    char value[80];
    if (jsonReadStringField(jsonData, jsonEnd, "updateTime", value, sizeof(value))) {
        data->updateTime = value;
    } else {
        data->updateTime = "";
    }

    const char* hourly = findWithin(jsonData, jsonEnd, "\"hourly\"");
    if (!hourly) {
        Serial.println("[Weather] Missing hourly array");
        return false;
    }

    const char* p = hourly;
    while (p < jsonEnd && *p != '[') p++;
    if (p >= jsonEnd) {
        Serial.println("[Weather] Invalid hourly array");
        return false;
    }
    p++;

    int total = 0;
    while (p < jsonEnd && total < hours) {
        const char* objStart = p;
        while (objStart < jsonEnd && *objStart != '{' && *objStart != ']') objStart++;
        if (objStart >= jsonEnd || *objStart == ']') {
            break;
        }

        const char* objEnd = objStart + 1;
        while (objEnd < jsonEnd && *objEnd != '}') objEnd++;
        if (objEnd >= jsonEnd) {
            break;
        }

        HourlyWeatherData& h = data->hours[total];

        jsonReadStringField(objStart, objEnd, "fxTime", value, sizeof(value));
        h.fxTime = value;
        jsonReadStringField(objStart, objEnd, "temp", value, sizeof(value));
        h.temp = value;
        jsonReadStringField(objStart, objEnd, "icon", value, sizeof(value));
        h.icon = value;
        if (jsonReadStringField(objStart, objEnd, "text", value, sizeof(value))) {
            h.text = value;
        } else {
            h.text = weatherGetIconChar(h.icon);
        }
        if (jsonReadStringField(objStart, objEnd, "windDir", value, sizeof(value))) {
            h.windDir = value;
        } else {
            h.windDir = "";
        }
        jsonReadStringField(objStart, objEnd, "windScale", value, sizeof(value));
        h.windScale = value;
        jsonReadStringField(objStart, objEnd, "humidity", value, sizeof(value));
        h.humidity = value;
        jsonReadStringField(objStart, objEnd, "pop", value, sizeof(value));
        h.pop = value;
        jsonReadStringField(objStart, objEnd, "precip", value, sizeof(value));
        h.precip = value;
        jsonReadStringField(objStart, objEnd, "pressure", value, sizeof(value));
        h.pressure = value;

        total++;
        p = objEnd + 1;
    }

    data->count = total;
    Serial.printf("[Weather] Parsed hourly count: %d\n", data->count);
    if (data->count == 0) {
        return false;
    }

    return true;
}

void weatherPrintData(const WeatherForecastData* data) {
    if (!data || data->count == 0) {
        Serial.println("[Weather] No data");
        return;
    }

    Serial.printf("[Weather] Update: %s\n", data->updateTime.c_str());
    for (int i = 0; i < data->count; i++) {
        const HourlyWeatherData& h = data->hours[i];
        Serial.printf("  [%d] %s | %sC icon=%s windScale=%s pop=%s%%\n",
            i, h.fxTime.c_str(), h.temp.c_str(), h.icon.c_str(),
            h.windScale.c_str(), h.pop.c_str());
    }
}

String weatherGetIconChar(const String& iconCode) {
    int code = iconCode.toInt();

    if (code >= 100 && code <= 103) return "SUN";
    if (code >= 104 && code <= 105) return "CLD";
    if (code >= 150 && code <= 153) return "SUN";

    if (code >= 300 && code <= 399) return "RAN";
    if (code >= 400 && code <= 499) return "SNW";
    if (code >= 500 && code <= 599) return "FOG";

    if (code >= 200 && code <= 299) return "WND";
    if (code >= 301 && code <= 305) return "SHW";

    return "---";
}
