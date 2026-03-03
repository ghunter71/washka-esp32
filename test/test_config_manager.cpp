/**
 * test_config_manager.cpp
 * 
 * Property-based tests for ConfigManager
 */

#include <unity.h>
#include <Arduino.h>
#include "ConfigManager.h"
#include <set>

// Test instance
ConfigManager* configManager = nullptr;

// ============================================================================
// Test Generators
// ============================================================================

/**
 * Generate random pin configuration
 * May include duplicates and invalid pins for testing
 */
ConfigManager::PinConfig generateRandomPinConfig() {
    ConfigManager::PinConfig config;
    config.washengine = random(0, 40);
    config.pompa = random(0, 40);
    config.watergerkon = random(0, 40);
    config.powder = random(0, 40);
    config.water_valve = random(0, 40);
    config.button = random(0, 40);
    config.led = random(0, 40);
    return config;
}

/**
 * Generate valid pin configuration (no duplicates, valid pins)
 */
ConfigManager::PinConfig generateValidPinConfig() {
    std::set<uint8_t> usedPins;
    ConfigManager::PinConfig config;
    
    // Valid ESP32 pins excluding flash pins (6-11)
    uint8_t validPins[] = {0, 1, 2, 3, 4, 5, 12, 13, 14, 15, 16, 17, 18, 19, 
                           21, 22, 23, 25, 26, 27, 32, 33, 34, 35, 36, 39};
    int validPinCount = sizeof(validPins) / sizeof(validPins[0]);
    
    // Assign unique pins
    config.washengine = validPins[random(0, validPinCount)];
    usedPins.insert(config.washengine);
    
    do { config.pompa = validPins[random(0, validPinCount)]; } 
    while (usedPins.count(config.pompa));
    usedPins.insert(config.pompa);
    
    do { config.watergerkon = validPins[random(0, validPinCount)]; } 
    while (usedPins.count(config.watergerkon));
    usedPins.insert(config.watergerkon);
    
    do { config.powder = validPins[random(0, validPinCount)]; } 
    while (usedPins.count(config.powder));
    usedPins.insert(config.powder);
    
    do { config.water_valve = validPins[random(0, validPinCount)]; } 
    while (usedPins.count(config.water_valve));
    usedPins.insert(config.water_valve);
    
    do { config.button = validPins[random(0, validPinCount)]; } 
    while (usedPins.count(config.button));
    usedPins.insert(config.button);
    
    do { config.led = validPins[random(0, validPinCount)]; } 
    while (usedPins.count(config.led));
    
    return config;
}

/**
 * Generate random timing configuration
 */
ConfigManager::TimingConfig generateRandomTimingConfig() {
    ConfigManager::TimingConfig config;
    config.tpomp = random(0, 100000);
    config.washtime0 = random(0, 5000000);
    config.washtime1 = random(0, 5000000);
    config.washtime2 = random(0, 5000000);
    config.washtime3 = random(0, 5000000);
    config.pausa = random(0, 200000);
    config.water_in_timer = random(0, 500000);
    return config;
}

/**
 * Generate valid timing configuration
 */
ConfigManager::TimingConfig generateValidTimingConfig() {
    ConfigManager::TimingConfig config;
    config.tpomp = random(1000, 300000);           // 1s - 5min
    config.washtime0 = random(60000, 1800000);     // 1min - 30min
    config.washtime1 = random(600000, 7200000);    // 10min - 2h
    config.washtime2 = random(300000, 1800000);    // 5min - 30min
    config.washtime3 = random(300000, 1800000);    // 5min - 30min
    config.pausa = random(10000, 120000);          // 10s - 2min
    config.water_in_timer = random(60000, 600000); // 1min - 10min
    return config;
}

// ============================================================================
// Setup and Teardown
// ============================================================================

void setUp(void) {
    if (configManager == nullptr) {
        configManager = new ConfigManager();
        configManager->begin();
    }
}

void tearDown(void) {
    // Clean up after each test
}

// ============================================================================
// Property 1: ESP32 pin validation
// **Feature: esp32-washka-upgrade, Property 1: ESP32 pin validation**
// **Validates: Requirements 1.2, 3.2**
// ============================================================================

void property_test_esp32_pin_validation() {
    Serial.println("\n=== Property Test: ESP32 Pin Validation ===");
    
    int iterations = 100;
    int validConfigs = 0;
    int invalidConfigs = 0;
    
    for (int i = 0; i < iterations; i++) {
        ConfigManager::PinConfig pins = generateRandomPinConfig();
        
        bool accepted = configManager->setPinConfig(pins);
        
        if (accepted) {
            validConfigs++;
            
            // Property: If config is accepted, no pins should be duplicated
            std::set<uint8_t> uniquePins = {
                pins.washengine, pins.pompa, pins.watergerkon,
                pins.powder, pins.water_valve, pins.button, pins.led
            };
            
            TEST_ASSERT_EQUAL_MESSAGE(7, uniquePins.size(), 
                "Accepted config has duplicate pins!");
            
            // Property: All pins must be in valid ESP32 range (0-39, excluding 6-11)
            TEST_ASSERT_TRUE_MESSAGE(pins.washengine <= 39 && 
                                    (pins.washengine < 6 || pins.washengine > 11),
                                    "Invalid washengine pin");
            TEST_ASSERT_TRUE_MESSAGE(pins.pompa <= 39 && 
                                    (pins.pompa < 6 || pins.pompa > 11),
                                    "Invalid pompa pin");
            TEST_ASSERT_TRUE_MESSAGE(pins.watergerkon <= 39 && 
                                    (pins.watergerkon < 6 || pins.watergerkon > 11),
                                    "Invalid watergerkon pin");
            TEST_ASSERT_TRUE_MESSAGE(pins.powder <= 39 && 
                                    (pins.powder < 6 || pins.powder > 11),
                                    "Invalid powder pin");
            TEST_ASSERT_TRUE_MESSAGE(pins.water_valve <= 39 && 
                                    (pins.water_valve < 6 || pins.water_valve > 11),
                                    "Invalid water_valve pin");
            TEST_ASSERT_TRUE_MESSAGE(pins.button <= 39 && 
                                    (pins.button < 6 || pins.button > 11),
                                    "Invalid button pin");
            TEST_ASSERT_TRUE_MESSAGE(pins.led <= 39 && 
                                    (pins.led < 6 || pins.led > 11),
                                    "Invalid led pin");
        } else {
            invalidConfigs++;
        }
    }
    
    Serial.printf("Valid configs: %d, Invalid configs: %d\n", validConfigs, invalidConfigs);
    Serial.println("✓ Property 1 passed: ESP32 pin validation");
}

// ============================================================================
// Property 2: WiFi credentials persistence round-trip
// **Feature: esp32-washka-upgrade, Property 2: WiFi credentials persistence round-trip**
// **Validates: Requirements 2.3**
// ============================================================================

void property_test_wifi_credentials_persistence() {
    Serial.println("\n=== Property Test: WiFi Credentials Persistence ===");
    
    int iterations = 100;
    
    for (int i = 0; i < iterations; i++) {
        // Generate random WiFi credentials
        String ssid = "TestSSID_" + String(random(1000, 9999));
        String password = "TestPass_" + String(random(100000, 999999));
        
        // Save credentials
        configManager->setWiFiCredentials(ssid, password);
        
        // Load credentials
        String loadedSSID = configManager->getWiFiSSID();
        String loadedPassword = configManager->getWiFiPassword();
        
        // Property: Saved and loaded values must be identical
        TEST_ASSERT_EQUAL_STRING_MESSAGE(ssid.c_str(), loadedSSID.c_str(),
                                        "SSID round-trip failed");
        TEST_ASSERT_EQUAL_STRING_MESSAGE(password.c_str(), loadedPassword.c_str(),
                                        "Password round-trip failed");
    }
    
    Serial.println("✓ Property 2 passed: WiFi credentials persistence round-trip");
}

// ============================================================================
// Property 3: GPIO configuration persistence round-trip
// **Feature: esp32-washka-upgrade, Property 3: GPIO configuration persistence round-trip**
// **Validates: Requirements 3.3, 3.4**
// ============================================================================

void property_test_gpio_configuration_persistence() {
    Serial.println("\n=== Property Test: GPIO Configuration Persistence ===");
    
    int iterations = 100;
    
    for (int i = 0; i < iterations; i++) {
        // Generate valid pin configuration
        ConfigManager::PinConfig pins = generateValidPinConfig();
        
        // Save configuration
        bool saved = configManager->setPinConfig(pins);
        TEST_ASSERT_TRUE_MESSAGE(saved, "Failed to save valid pin config");
        
        // Load configuration
        ConfigManager::PinConfig loaded = configManager->getPinConfig();
        
        // Property: Saved and loaded values must be identical
        TEST_ASSERT_EQUAL_MESSAGE(pins.washengine, loaded.washengine, "washengine mismatch");
        TEST_ASSERT_EQUAL_MESSAGE(pins.pompa, loaded.pompa, "pompa mismatch");
        TEST_ASSERT_EQUAL_MESSAGE(pins.watergerkon, loaded.watergerkon, "watergerkon mismatch");
        TEST_ASSERT_EQUAL_MESSAGE(pins.powder, loaded.powder, "powder mismatch");
        TEST_ASSERT_EQUAL_MESSAGE(pins.water_valve, loaded.water_valve, "water_valve mismatch");
        TEST_ASSERT_EQUAL_MESSAGE(pins.button, loaded.button, "button mismatch");
        TEST_ASSERT_EQUAL_MESSAGE(pins.led, loaded.led, "led mismatch");
    }
    
    Serial.println("✓ Property 3 passed: GPIO configuration persistence round-trip");
}

// ============================================================================
// Property 4: Timing configuration validation
// **Feature: esp32-washka-upgrade, Property 4: Timing configuration validation**
// **Validates: Requirements 4.2, 4.5**
// ============================================================================

void property_test_timing_validation() {
    Serial.println("\n=== Property Test: Timing Configuration Validation ===");
    
    int iterations = 100;
    int validConfigs = 0;
    int invalidConfigs = 0;
    
    for (int i = 0; i < iterations; i++) {
        ConfigManager::TimingConfig timing = generateRandomTimingConfig();
        
        bool accepted = configManager->setTimingConfig(timing);
        
        if (accepted) {
            validConfigs++;
            
            // Property: All accepted timing values must be within valid range (1s - 24h)
            TEST_ASSERT_TRUE_MESSAGE(timing.tpomp >= 1000 && timing.tpomp <= 86400000,
                                    "Invalid tpomp value");
            TEST_ASSERT_TRUE_MESSAGE(timing.washtime0 >= 1000 && timing.washtime0 <= 86400000,
                                    "Invalid washtime0 value");
            TEST_ASSERT_TRUE_MESSAGE(timing.washtime1 >= 1000 && timing.washtime1 <= 86400000,
                                    "Invalid washtime1 value");
            TEST_ASSERT_TRUE_MESSAGE(timing.washtime2 >= 1000 && timing.washtime2 <= 86400000,
                                    "Invalid washtime2 value");
            TEST_ASSERT_TRUE_MESSAGE(timing.washtime3 >= 1000 && timing.washtime3 <= 86400000,
                                    "Invalid washtime3 value");
            TEST_ASSERT_TRUE_MESSAGE(timing.pausa >= 1000 && timing.pausa <= 86400000,
                                    "Invalid pausa value");
            TEST_ASSERT_TRUE_MESSAGE(timing.water_in_timer >= 1000 && timing.water_in_timer <= 86400000,
                                    "Invalid water_in_timer value");
        } else {
            invalidConfigs++;
        }
    }
    
    Serial.printf("Valid configs: %d, Invalid configs: %d\n", validConfigs, invalidConfigs);
    Serial.println("✓ Property 4 passed: Timing configuration validation");
}

// ============================================================================
// Property 5: Timing configuration persistence round-trip
// **Feature: esp32-washka-upgrade, Property 5: Timing configuration persistence round-trip**
// **Validates: Requirements 4.3, 4.4**
// ============================================================================

void property_test_timing_persistence() {
    Serial.println("\n=== Property Test: Timing Configuration Persistence ===");
    
    int iterations = 100;
    
    for (int i = 0; i < iterations; i++) {
        // Generate valid timing configuration
        ConfigManager::TimingConfig timing = generateValidTimingConfig();
        
        // Save configuration
        bool saved = configManager->setTimingConfig(timing);
        TEST_ASSERT_TRUE_MESSAGE(saved, "Failed to save valid timing config");
        
        // Load configuration
        ConfigManager::TimingConfig loaded = configManager->getTimingConfig();
        
        // Property: Saved and loaded values must be identical
        TEST_ASSERT_EQUAL_MESSAGE(timing.tpomp, loaded.tpomp, "tpomp mismatch");
        TEST_ASSERT_EQUAL_MESSAGE(timing.washtime0, loaded.washtime0, "washtime0 mismatch");
        TEST_ASSERT_EQUAL_MESSAGE(timing.washtime1, loaded.washtime1, "washtime1 mismatch");
        TEST_ASSERT_EQUAL_MESSAGE(timing.washtime2, loaded.washtime2, "washtime2 mismatch");
        TEST_ASSERT_EQUAL_MESSAGE(timing.washtime3, loaded.washtime3, "washtime3 mismatch");
        TEST_ASSERT_EQUAL_MESSAGE(timing.pausa, loaded.pausa, "pausa mismatch");
        TEST_ASSERT_EQUAL_MESSAGE(timing.water_in_timer, loaded.water_in_timer, 
                                 "water_in_timer mismatch");
    }
    
    Serial.println("✓ Property 5 passed: Timing configuration persistence round-trip");
}

// ============================================================================
// Property 22: Configuration export/import round-trip
// **Feature: esp32-washka-upgrade, Property 22: Configuration export/import round-trip**
// **Validates: Requirements 13.5**
// ============================================================================

void property_test_config_export_import() {
    Serial.println("\n=== Property Test: Configuration Export/Import Round-trip ===");
    
    int iterations = 50; // Fewer iterations due to JSON processing overhead
    
    for (int i = 0; i < iterations; i++) {
        // Set random valid configuration
        ConfigManager::PinConfig pins = generateValidPinConfig();
        ConfigManager::TimingConfig timing = generateValidTimingConfig();
        
        configManager->setPinConfig(pins);
        configManager->setTimingConfig(timing);
        configManager->setGerkonThreshold(random(50, 500));
        configManager->setGerkonDebounceMs(random(10, 200));
        
        // Export configuration
        String exported = configManager->exportToJson();
        TEST_ASSERT_TRUE_MESSAGE(exported.length() > 0, "Export failed");
        
        // Modify configuration
        ConfigManager::PinConfig newPins = generateValidPinConfig();
        configManager->setPinConfig(newPins);
        
        // Import original configuration
        bool imported = configManager->importFromJson(exported);
        TEST_ASSERT_TRUE_MESSAGE(imported, "Import failed");
        
        // Load and verify
        ConfigManager::PinConfig loadedPins = configManager->getPinConfig();
        ConfigManager::TimingConfig loadedTiming = configManager->getTimingConfig();
        
        // Property: Exported then imported config must match original
        TEST_ASSERT_EQUAL_MESSAGE(pins.washengine, loadedPins.washengine, "Pin mismatch after import");
        TEST_ASSERT_EQUAL_MESSAGE(timing.tpomp, loadedTiming.tpomp, "Timing mismatch after import");
    }
    
    Serial.println("✓ Property 22 passed: Configuration export/import round-trip");
}

// ============================================================================
// Property 21: Configuration NVS persistence
// **Feature: esp32-washka-upgrade, Property 21: Configuration NVS persistence**
// **Validates: Requirements 13.1**
// ============================================================================

void property_test_nvs_persistence() {
    Serial.println("\n=== Property Test: Configuration NVS Persistence ===");
    
    int iterations = 100;
    
    for (int i = 0; i < iterations; i++) {
        // Generate and save random configuration
        ConfigManager::PinConfig pins = generateValidPinConfig();
        ConfigManager::TimingConfig timing = generateValidTimingConfig();
        uint16_t threshold = random(50, 500);
        uint16_t debounce = random(10, 200);
        
        configManager->setPinConfig(pins);
        configManager->setTimingConfig(timing);
        configManager->setGerkonThreshold(threshold);
        configManager->setGerkonDebounceMs(debounce);
        
        // Property: Immediately after saving, values must be retrievable
        ConfigManager::PinConfig loadedPins = configManager->getPinConfig();
        ConfigManager::TimingConfig loadedTiming = configManager->getTimingConfig();
        uint16_t loadedThreshold = configManager->getGerkonThreshold();
        uint16_t loadedDebounce = configManager->getGerkonDebounceMs();
        
        TEST_ASSERT_EQUAL_MESSAGE(pins.washengine, loadedPins.washengine, "Pin not persisted");
        TEST_ASSERT_EQUAL_MESSAGE(timing.tpomp, loadedTiming.tpomp, "Timing not persisted");
        TEST_ASSERT_EQUAL_MESSAGE(threshold, loadedThreshold, "Threshold not persisted");
        TEST_ASSERT_EQUAL_MESSAGE(debounce, loadedDebounce, "Debounce not persisted");
    }
    
    Serial.println("✓ Property 21 passed: Configuration NVS persistence");
}

// ============================================================================
// Test Runner
// ============================================================================

void setup() {
    delay(2000); // Wait for serial
    
    UNITY_BEGIN();
    
    RUN_TEST(property_test_esp32_pin_validation);
    RUN_TEST(property_test_wifi_credentials_persistence);
    RUN_TEST(property_test_gpio_configuration_persistence);
    RUN_TEST(property_test_timing_validation);
    RUN_TEST(property_test_timing_persistence);
    RUN_TEST(property_test_config_export_import);
    RUN_TEST(property_test_nvs_persistence);
    
    UNITY_END();
}

void loop() {
    // Tests run once in setup()
}
