/**
 * @file preferences.cpp
 * @brief NVS preferences implementation
 */

#include "preferences.h"
#include <Preferences.h>

static Preferences preferences;
static bool _initialized = false;

void prefsInit(void) {
    if (!_initialized) {
        preferences.begin(PREF_NAMESPACE, false);  // false = 读写模式
        _initialized = true;
    }
}

bool prefsPutString(const char* key, const char* value) {
    if (!_initialized) prefsInit();
    return preferences.putString(key, value) > 0;
}

size_t prefsGetString(const char* key, char* buffer, size_t maxLen) {
    if (!_initialized) prefsInit();
    String value = preferences.getString(key, "");
    if (value.length() == 0) return 0;
    
    size_t len = value.length();
    if (len >= maxLen) len = maxLen - 1;
    
    strncpy(buffer, value.c_str(), len);
    buffer[len] = '\0';
    return len;
}

bool prefsPutInt(const char* key, int32_t value) {
    if (!_initialized) prefsInit();
    return preferences.putInt(key, value) > 0;
}

int32_t prefsGetInt(const char* key, int32_t defaultValue) {
    if (!_initialized) prefsInit();
    return preferences.getInt(key, defaultValue);
}

bool prefsPutBool(const char* key, bool value) {
    if (!_initialized) prefsInit();
    return preferences.putBool(key, value) > 0;
}

bool prefsGetBool(const char* key, bool defaultValue) {
    if (!_initialized) prefsInit();
    return preferences.getBool(key, defaultValue);
}

bool prefsExists(const char* key) {
    if (!_initialized) prefsInit();
    return preferences.isKey(key);
}

bool prefsRemove(const char* key) {
    if (!_initialized) prefsInit();
    return preferences.remove(key);
}

bool prefsClear(void) {
    if (!_initialized) prefsInit();
    return preferences.clear();
}
