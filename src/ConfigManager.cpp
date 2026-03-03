/**
 * ConfigManager.cpp
 * 
 * Implementation of configuration management with NVS persistence.
 */

#include "ConfigManager.h"

// NVS Key definitions
const char* ConfigManager::KEY_WIFI_SSID = "wifi_ssid";
const char* ConfigManager::KEY_WIFI_PASS = "wifi_pass";
const char* ConfigManager::KEY_PIN_WASHENGINE = "pin_wash";
const char* ConfigManager::KEY_PIN_POMPA = "pin_pompa";
const char* ConfigManager::KEY_PIN_GERKON = "pin_gerkon";
const char* ConfigManager::KEY_PIN_POWDER = "pin_powder";
const char* ConfigManager::KEY_PIN_VALVE = "pin_valve";
const char* ConfigManager::KEY_PIN_BUTTON = "pin_button";
const char* ConfigManager::KEY_PIN_LED = "pin_led";
const char* ConfigManager::KEY_TIME_PUMP = "time_pump";
const char* ConfigManager::KEY_TIME_PREWASH = "time_prewash";
const char* ConfigManager::KEY_TIME_WASH = "time_wash";
const char* ConfigManager::KEY_TIME_RINSE1 = "time_rinse1";
const char* ConfigManager::KEY_TIME_RINSE2 = "time_rinse2";
const char* ConfigManager::KEY_TIME_PAUSE = "time_pause";
const char* ConfigManager::KEY_TIME_WATER = "time_water";
const char* ConfigManager::KEY_GERKON_THRESHOLD = "gerk_thresh";
const char* ConfigManager::KEY_GERKON_DEBOUNCE = "gerk_debounce";
const char* ConfigManager::KEY_TG_ENABLED = "tg_enabled";
const char* ConfigManager::KEY_TG_TOKEN = "tg_token";
const char* ConfigManager::KEY_TG_CHAT_IDS = "tg_chats";

ConfigManager::ConfigManager() {
}

ConfigManager::~ConfigManager() {
    prefs.end();
}

bool ConfigManager::begin() {
    if (!prefs.begin(NVS_NAMESPACE, false)) {
        Serial.println("ERROR: Failed to initialize ConfigManager NVS");
        return false;
    }
    
    Serial.println("✓ ConfigManager initialized");
    return true;
}

// ============================================================================
// WiFi Settings
// ============================================================================

String ConfigManager::getWiFiSSID() {
    return prefs.getString(KEY_WIFI_SSID, "");
}

String ConfigManager::getWiFiPassword() {
    return prefs.getString(KEY_WIFI_PASS, "");
}

void ConfigManager::setWiFiCredentials(const String& ssid, const String& password) {
    prefs.putString(KEY_WIFI_SSID, ssid);
    prefs.putString(KEY_WIFI_PASS, password);
    Serial.printf("✓ WiFi credentials saved: %s\n", ssid.c_str());
}

bool ConfigManager::hasWiFiCredentials() {
    return prefs.isKey(KEY_WIFI_SSID) && getWiFiSSID().length() > 0;
}

// ============================================================================
// GPIO Configuration
// ============================================================================

ConfigManager::PinConfig ConfigManager::getPinConfig() {
    PinConfig config;
    config.washengine = prefs.getUChar(KEY_PIN_WASHENGINE, DEFAULT_WASHENGINE_PIN);
    config.pompa = prefs.getUChar(KEY_PIN_POMPA, DEFAULT_POMPA_PIN);
    config.watergerkon = prefs.getUChar(KEY_PIN_GERKON, DEFAULT_GERKON_PIN);
    config.powder = prefs.getUChar(KEY_PIN_POWDER, DEFAULT_POWDER_PIN);
    config.water_valve = prefs.getUChar(KEY_PIN_VALVE, DEFAULT_VALVE_PIN);
    config.button = prefs.getUChar(KEY_PIN_BUTTON, DEFAULT_BUTTON_PIN);
    config.led = prefs.getUChar(KEY_PIN_LED, DEFAULT_LED_PIN);
    return config;
}

bool ConfigManager::setPinConfig(const PinConfig& config) {
    if (!validatePinConfig(config)) {
        Serial.println("ERROR: Invalid pin configuration");
        return false;
    }
    
    prefs.putUChar(KEY_PIN_WASHENGINE, config.washengine);
    prefs.putUChar(KEY_PIN_POMPA, config.pompa);
    prefs.putUChar(KEY_PIN_GERKON, config.watergerkon);
    prefs.putUChar(KEY_PIN_POWDER, config.powder);
    prefs.putUChar(KEY_PIN_VALVE, config.water_valve);
    prefs.putUChar(KEY_PIN_BUTTON, config.button);
    prefs.putUChar(KEY_PIN_LED, config.led);
    
    Serial.println("✓ Pin configuration saved");
    return true;
}

bool ConfigManager::validatePinConfig(const PinConfig& config) {
    // Check for duplicate pins
    if (hasDuplicatePins(config)) {
        Serial.println("ERROR: Duplicate pin assignments detected");
        return false;
    }
    
    // Validate each pin
    if (!isPinValid(config.washengine) || !isPinValid(config.pompa) ||
        !isPinValid(config.watergerkon) || !isPinValid(config.powder) ||
        !isPinValid(config.water_valve) || !isPinValid(config.button) ||
        !isPinValid(config.led)) {
        Serial.println("ERROR: Invalid pin number detected");
        return false;
    }
    
    return true;
}

bool ConfigManager::hasDuplicatePins(const PinConfig& config) {
    uint8_t pins[] = {
        config.washengine, config.pompa, config.watergerkon,
        config.powder, config.water_valve, config.button, config.led
    };
    
    for (int i = 0; i < 7; i++) {
        for (int j = i + 1; j < 7; j++) {
            if (pins[i] == pins[j]) {
                return true;
            }
        }
    }
    return false;
}

bool ConfigManager::isPinValid(uint8_t pin) {
    // Use PinLegend for comprehensive validation
    return PinLegend::isPinValid(pin);
}

// ============================================================================
// Timing Configuration
// ============================================================================

ConfigManager::TimingConfig ConfigManager::getTimingConfig() {
    TimingConfig config;
    config.tpomp = prefs.getULong(KEY_TIME_PUMP, DEFAULT_PUMP_TIME);
    config.washtime0 = prefs.getULong(KEY_TIME_PREWASH, DEFAULT_PREWASH_TIME);
    config.washtime1 = prefs.getULong(KEY_TIME_WASH, DEFAULT_WASH_TIME);
    config.washtime2 = prefs.getULong(KEY_TIME_RINSE1, DEFAULT_RINSE1_TIME);
    config.washtime3 = prefs.getULong(KEY_TIME_RINSE2, DEFAULT_RINSE2_TIME);
    config.pausa = prefs.getULong(KEY_TIME_PAUSE, DEFAULT_PAUSE_TIME);
    config.water_in_timer = prefs.getULong(KEY_TIME_WATER, DEFAULT_WATER_TIMEOUT);
    return config;
}

bool ConfigManager::setTimingConfig(const TimingConfig& config) {
    if (!validateTimingConfig(config)) {
        Serial.println("ERROR: Invalid timing configuration");
        return false;
    }
    
    prefs.putULong(KEY_TIME_PUMP, config.tpomp);
    prefs.putULong(KEY_TIME_PREWASH, config.washtime0);
    prefs.putULong(KEY_TIME_WASH, config.washtime1);
    prefs.putULong(KEY_TIME_RINSE1, config.washtime2);
    prefs.putULong(KEY_TIME_RINSE2, config.washtime3);
    prefs.putULong(KEY_TIME_PAUSE, config.pausa);
    prefs.putULong(KEY_TIME_WATER, config.water_in_timer);
    
    Serial.println("✓ Timing configuration saved");
    return true;
}

bool ConfigManager::validateTimingConfig(const TimingConfig& config) {
    // Check all timing values are valid (not zero, not too large)
    if (!isTimingValid(config.tpomp) || !isTimingValid(config.washtime0) ||
        !isTimingValid(config.washtime1) || !isTimingValid(config.washtime2) ||
        !isTimingValid(config.washtime3) || !isTimingValid(config.pausa) ||
        !isTimingValid(config.water_in_timer)) {
        Serial.println("ERROR: Invalid timing value detected");
        return false;
    }
    
    return true;
}

bool ConfigManager::isTimingValid(unsigned long value) {
    // Timing must be between 1 second and 24 hours
    return (value >= 1000 && value <= 86400000);
}

// ============================================================================
// Gerkon Configuration
// ============================================================================

uint16_t ConfigManager::getGerkonThreshold() {
    return prefs.getUShort(KEY_GERKON_THRESHOLD, DEFAULT_GERKON_THRESHOLD);
}

void ConfigManager::setGerkonThreshold(uint16_t threshold) {
    prefs.putUShort(KEY_GERKON_THRESHOLD, threshold);
    Serial.printf("✓ Gerkon threshold set to: %d\n", threshold);
}

uint16_t ConfigManager::getGerkonDebounceMs() {
    return prefs.getUShort(KEY_GERKON_DEBOUNCE, DEFAULT_GERKON_DEBOUNCE);
}

void ConfigManager::setGerkonDebounceMs(uint16_t ms) {
    prefs.putUShort(KEY_GERKON_DEBOUNCE, ms);
    Serial.printf("✓ Gerkon debounce set to: %d ms\n", ms);
}

// ============================================================================
// Telegram Configuration
// ============================================================================

bool ConfigManager::isTelegramEnabled() {
    return prefs.getBool(KEY_TG_ENABLED, false);  // Default: disabled
}

void ConfigManager::setTelegramEnabled(bool enabled) {
    prefs.putBool(KEY_TG_ENABLED, enabled);
    Serial.printf("✓ Telegram %s\n", enabled ? "enabled" : "disabled");
}

String ConfigManager::getTelegramToken() {
    return prefs.getString(KEY_TG_TOKEN, "");
}

void ConfigManager::setTelegramToken(const String& token) {
    prefs.putString(KEY_TG_TOKEN, token);
    Serial.println("✓ Telegram token saved");
}

std::vector<int64_t> ConfigManager::getAllowedChatIds() {
    std::vector<int64_t> chatIds;
    String chatIdsStr = prefs.getString(KEY_TG_CHAT_IDS, "");
    
    if (chatIdsStr.length() > 0) {
        // Parse comma-separated chat IDs
        int startIdx = 0;
        int commaIdx = chatIdsStr.indexOf(',');
        
        while (commaIdx != -1) {
            String idStr = chatIdsStr.substring(startIdx, commaIdx);
            chatIds.push_back(idStr.toInt());
            startIdx = commaIdx + 1;
            commaIdx = chatIdsStr.indexOf(',', startIdx);
        }
        
        // Add last ID
        if (startIdx < chatIdsStr.length()) {
            String idStr = chatIdsStr.substring(startIdx);
            chatIds.push_back(idStr.toInt());
        }
    }
    
    return chatIds;
}

void ConfigManager::addAllowedChatId(int64_t chatId) {
    std::vector<int64_t> chatIds = getAllowedChatIds();
    
    // Check if already exists
    for (int64_t id : chatIds) {
        if (id == chatId) {
            Serial.println("Chat ID already in allowed list");
            return;
        }
    }
    
    chatIds.push_back(chatId);
    
    // Save back to NVS
    String chatIdsStr = "";
    for (size_t i = 0; i < chatIds.size(); i++) {
        if (i > 0) chatIdsStr += ",";
        chatIdsStr += String((long)chatIds[i]);
    }
    
    prefs.putString(KEY_TG_CHAT_IDS, chatIdsStr);
    Serial.printf("✓ Chat ID %lld added to allowed list\n", chatId);
}

void ConfigManager::removeAllowedChatId(int64_t chatId) {
    std::vector<int64_t> chatIds = getAllowedChatIds();
    std::vector<int64_t> newChatIds;
    
    for (int64_t id : chatIds) {
        if (id != chatId) {
            newChatIds.push_back(id);
        }
    }
    
    // Save back to NVS
    String chatIdsStr = "";
    for (size_t i = 0; i < newChatIds.size(); i++) {
        if (i > 0) chatIdsStr += ",";
        chatIdsStr += String((long)newChatIds[i]);
    }
    
    prefs.putString(KEY_TG_CHAT_IDS, chatIdsStr);
    Serial.printf("✓ Chat ID %lld removed from allowed list\n", chatId);
}

void ConfigManager::clearAllowedChatIds() {
    prefs.remove(KEY_TG_CHAT_IDS);
    Serial.println("✓ All chat IDs cleared");
}

// ============================================================================
// Factory Reset
// ============================================================================

void ConfigManager::factoryReset() {
    Serial.println("Performing factory reset...");
    prefs.clear();
    Serial.println("✓ All configuration cleared");
}

// ============================================================================
// Export/Import
// ============================================================================

String ConfigManager::exportToJson() {
    DynamicJsonDocument doc(2048);
    
    // WiFi
    doc["wifi"]["ssid"] = getWiFiSSID();
    // Don't export password for security
    
    // Pins
    PinConfig pins = getPinConfig();
    doc["pins"]["washengine"] = pins.washengine;
    doc["pins"]["pompa"] = pins.pompa;
    doc["pins"]["watergerkon"] = pins.watergerkon;
    doc["pins"]["powder"] = pins.powder;
    doc["pins"]["water_valve"] = pins.water_valve;
    doc["pins"]["button"] = pins.button;
    doc["pins"]["led"] = pins.led;
    
    // Timing
    TimingConfig timing = getTimingConfig();
    doc["timing"]["tpomp"] = timing.tpomp;
    doc["timing"]["washtime0"] = timing.washtime0;
    doc["timing"]["washtime1"] = timing.washtime1;
    doc["timing"]["washtime2"] = timing.washtime2;
    doc["timing"]["washtime3"] = timing.washtime3;
    doc["timing"]["pausa"] = timing.pausa;
    doc["timing"]["water_in_timer"] = timing.water_in_timer;
    
    // Gerkon
    doc["gerkon"]["threshold"] = getGerkonThreshold();
    doc["gerkon"]["debounce"] = getGerkonDebounceMs();
    
    // Telegram (don't export token for security)
    JsonArray chatIds = doc.createNestedArray("telegram_chat_ids");
    for (int64_t id : getAllowedChatIds()) {
        chatIds.add((long)id);
    }
    
    String output;
    serializeJsonPretty(doc, output);
    return output;
}

bool ConfigManager::importFromJson(const String& json) {
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, json);
    
    if (error) {
        Serial.printf("ERROR: Failed to parse JSON: %s\n", error.c_str());
        return false;
    }
    
    // Import WiFi (if present)
    if (doc.containsKey("wifi") && doc["wifi"].containsKey("ssid")) {
        String ssid = doc["wifi"]["ssid"].as<String>();
        String pass = doc["wifi"].containsKey("password") ? 
                      doc["wifi"]["password"].as<String>() : "";
        if (ssid.length() > 0) {
            setWiFiCredentials(ssid, pass);
        }
    }
    
    // Import pins
    if (doc.containsKey("pins")) {
        PinConfig pins;
        pins.washengine = doc["pins"]["washengine"] | DEFAULT_WASHENGINE_PIN;
        pins.pompa = doc["pins"]["pompa"] | DEFAULT_POMPA_PIN;
        pins.watergerkon = doc["pins"]["watergerkon"] | DEFAULT_GERKON_PIN;
        pins.powder = doc["pins"]["powder"] | DEFAULT_POWDER_PIN;
        pins.water_valve = doc["pins"]["water_valve"] | DEFAULT_VALVE_PIN;
        pins.button = doc["pins"]["button"] | DEFAULT_BUTTON_PIN;
        pins.led = doc["pins"]["led"] | DEFAULT_LED_PIN;
        setPinConfig(pins);
    }
    
    // Import timing
    if (doc.containsKey("timing")) {
        TimingConfig timing;
        timing.tpomp = doc["timing"]["tpomp"] | DEFAULT_PUMP_TIME;
        timing.washtime0 = doc["timing"]["washtime0"] | DEFAULT_PREWASH_TIME;
        timing.washtime1 = doc["timing"]["washtime1"] | DEFAULT_WASH_TIME;
        timing.washtime2 = doc["timing"]["washtime2"] | DEFAULT_RINSE1_TIME;
        timing.washtime3 = doc["timing"]["washtime3"] | DEFAULT_RINSE2_TIME;
        timing.pausa = doc["timing"]["pausa"] | DEFAULT_PAUSE_TIME;
        timing.water_in_timer = doc["timing"]["water_in_timer"] | DEFAULT_WATER_TIMEOUT;
        setTimingConfig(timing);
    }
    
    // Import gerkon
    if (doc.containsKey("gerkon")) {
        if (doc["gerkon"].containsKey("threshold")) {
            setGerkonThreshold(doc["gerkon"]["threshold"]);
        }
        if (doc["gerkon"].containsKey("debounce")) {
            setGerkonDebounceMs(doc["gerkon"]["debounce"]);
        }
    }
    
    // Import Telegram chat IDs
    if (doc.containsKey("telegram_chat_ids")) {
        clearAllowedChatIds();
        JsonArray chatIds = doc["telegram_chat_ids"];
        for (JsonVariant id : chatIds) {
            addAllowedChatId(id.as<long>());
        }
    }
    
    Serial.println("✓ Configuration imported from JSON");
    return true;
}
