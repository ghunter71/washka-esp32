/**
 * ConfigManager.h
 * 
 * Manages all system configuration with NVS (Non-Volatile Storage) persistence.
 * Handles WiFi credentials, GPIO pins, timing parameters, gerkon settings, and Telegram configuration.
 */

#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>
#include <vector>
#include <ArduinoJson.h>
#include "PinLegend.h"

// NVS namespace
#define NVS_NAMESPACE "washka"

// Default values
#define DEFAULT_WASHENGINE_PIN 5
#define DEFAULT_POMPA_PIN 4
#define DEFAULT_GERKON_PIN 13
#define DEFAULT_POWDER_PIN 12
#define DEFAULT_VALVE_PIN 14
#define DEFAULT_BUTTON_PIN 2
#define DEFAULT_LED_PIN 15

#define DEFAULT_PUMP_TIME 45000UL
#define DEFAULT_PREWASH_TIME 600000UL
#define DEFAULT_WASH_TIME 2400000UL
#define DEFAULT_RINSE1_TIME 600000UL
#define DEFAULT_RINSE2_TIME 600000UL
#define DEFAULT_PAUSE_TIME 60000UL
#define DEFAULT_WATER_TIMEOUT 180000UL

#define DEFAULT_GERKON_THRESHOLD 210
#define DEFAULT_GERKON_DEBOUNCE 50

class ConfigManager {
public:
    // GPIO Pin Configuration
    struct PinConfig {
        uint8_t washengine;
        uint8_t pompa;
        uint8_t watergerkon;
        uint8_t powder;
        uint8_t water_valve;
        uint8_t button;
        uint8_t led;
        
        PinConfig() : 
            washengine(DEFAULT_WASHENGINE_PIN),
            pompa(DEFAULT_POMPA_PIN),
            watergerkon(DEFAULT_GERKON_PIN),
            powder(DEFAULT_POWDER_PIN),
            water_valve(DEFAULT_VALVE_PIN),
            button(DEFAULT_BUTTON_PIN),
            led(DEFAULT_LED_PIN) {}
    };
    
    // Timing Configuration (all in milliseconds)
    struct TimingConfig {
        unsigned long tpomp;           // Pump duration
        unsigned long washtime0;       // Pre-wash time
        unsigned long washtime1;       // Main wash time
        unsigned long washtime2;       // First rinse time
        unsigned long washtime3;       // Second rinse time
        unsigned long pausa;           // Pause before drain
        unsigned long water_in_timer;  // Water fill timeout
        
        TimingConfig() :
            tpomp(DEFAULT_PUMP_TIME),
            washtime0(DEFAULT_PREWASH_TIME),
            washtime1(DEFAULT_WASH_TIME),
            washtime2(DEFAULT_RINSE1_TIME),
            washtime3(DEFAULT_RINSE2_TIME),
            pausa(DEFAULT_PAUSE_TIME),
            water_in_timer(DEFAULT_WATER_TIMEOUT) {}
    };
    
    ConfigManager();
    ~ConfigManager();
    
    // Initialization
    bool begin();
    
    // WiFi Settings
    String getWiFiSSID();
    String getWiFiPassword();
    void setWiFiCredentials(const String& ssid, const String& password);
    bool hasWiFiCredentials();
    
    // GPIO Configuration
    PinConfig getPinConfig();
    bool setPinConfig(const PinConfig& config);
    bool validatePinConfig(const PinConfig& config);
    
    // Timing Configuration
    TimingConfig getTimingConfig();
    bool setTimingConfig(const TimingConfig& config);
    bool validateTimingConfig(const TimingConfig& config);
    
    // Gerkon Configuration
    uint16_t getGerkonThreshold();
    void setGerkonThreshold(uint16_t threshold);
    uint16_t getGerkonDebounceMs();
    void setGerkonDebounceMs(uint16_t ms);
    
    // Telegram Configuration
    bool isTelegramEnabled();
    void setTelegramEnabled(bool enabled);
    String getTelegramToken();
    void setTelegramToken(const String& token);
    std::vector<int64_t> getAllowedChatIds();
    void addAllowedChatId(int64_t chatId);
    void removeAllowedChatId(int64_t chatId);
    void clearAllowedChatIds();
    
    // Factory Reset
    void factoryReset();
    
    // Export/Import
    String exportToJson();
    bool importFromJson(const String& json);
    
private:
    Preferences prefs;
    
    // Helper methods
    bool hasDuplicatePins(const PinConfig& config);
    bool isPinValid(uint8_t pin);
    bool isTimingValid(unsigned long value);
    
    // NVS Keys
    static const char* KEY_WIFI_SSID;
    static const char* KEY_WIFI_PASS;
    static const char* KEY_PIN_WASHENGINE;
    static const char* KEY_PIN_POMPA;
    static const char* KEY_PIN_GERKON;
    static const char* KEY_PIN_POWDER;
    static const char* KEY_PIN_VALVE;
    static const char* KEY_PIN_BUTTON;
    static const char* KEY_PIN_LED;
    static const char* KEY_TIME_PUMP;
    static const char* KEY_TIME_PREWASH;
    static const char* KEY_TIME_WASH;
    static const char* KEY_TIME_RINSE1;
    static const char* KEY_TIME_RINSE2;
    static const char* KEY_TIME_PAUSE;
    static const char* KEY_TIME_WATER;
    static const char* KEY_GERKON_THRESHOLD;
    static const char* KEY_GERKON_DEBOUNCE;
    static const char* KEY_TG_ENABLED;
    static const char* KEY_TG_TOKEN;
    static const char* KEY_TG_CHAT_IDS;
};

#endif // CONFIG_MANAGER_H
