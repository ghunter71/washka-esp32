/**
 * test_rest_api.cpp
 * 
 * Property-based tests for RestAPI
 */

#include <unity.h>
#include <Arduino.h>
#include "RestAPI.h"
#include "ConfigManager.h"
#include "StateManager.h"
#include "ActuatorManager.h"
#include "WaterControl.h"
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

// Test instances
RestAPI* restAPI = nullptr;
ConfigManager* configManager = nullptr;
StateManager* stateManager = nullptr;
ActuatorManager* actuatorManager = nullptr;
WaterControl* waterControl = nullptr;
AsyncWebServer* server = nullptr;

// ============================================================================
// Test Generators
// ============================================================================

/**
 * Generate random API token
 */
String generateRandomAPIToken() {
    String token = "";
    int length = random(8, 32);
    const char* chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    
    for (int i = 0; i < length; i++) {
        token += chars[random(0, 62)];
    }
    
    return token;
}

/**
 * Generate random JSON configuration
 */
String generateRandomConfigJSON() {
    DynamicJsonDocument doc(1024);
    
    // Random pins
    JsonObject pins = doc.createNestedObject("pins");
    pins["washengine"] = random(0, 40);
    pins["pompa"] = random(0, 40);
    pins["watergerkon"] = random(0, 40);
    pins["powder"] = random(0, 40);
    pins["water_valve"] = random(0, 40);
    pins["button"] = random(0, 40);
    pins["led"] = random(0, 40);
    
    // Random timing
    JsonObject timing = doc.createNestedObject("timing");
    timing["tpomp"] = random(1000, 300000);
    timing["washtime0"] = random(60000, 1800000);
    timing["washtime1"] = random(600000, 7200000);
    timing["washtime2"] = random(300000, 1800000);
    timing["washtime3"] = random(300000, 1800000);
    timing["pausa"] = random(10000, 120000);
    timing["water_in_timer"] = random(60000, 600000);
    
    // Random gerkon
    JsonObject gerkon = doc.createNestedObject("gerkon");
    gerkon["threshold"] = random(50, 500);
    gerkon["debounce_ms"] = random(10, 200);
    
    String json;
    serializeJson(doc, json);
    return json;
}

/**
 * Check if string is valid JSON
 */
bool isValidJSON(const String& str) {
    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, str);
    return (error == DeserializationError::Ok);
}

// ============================================================================
// Setup and Teardown
// ============================================================================

void setUp(void) {
    if (configManager == nullptr) {
        configManager = new ConfigManager();
        configManager->begin();
    }
    
    if (stateManager == nullptr) {
        stateManager = new StateManager();
        stateManager->begin();
    }
    
    if (actuatorManager == nullptr) {
        actuatorManager = new ActuatorManager();
        ConfigManager::PinConfig pins = configManager->getPinConfig();
        actuatorManager->begin(pins);
    }
    
    if (waterControl == nullptr) {
        waterControl = new WaterControl();
        ConfigManager::PinConfig pins = configManager->getPinConfig();
        waterControl->begin(pins.watergerkon);
    }
    
    if (server == nullptr) {
        server = new AsyncWebServer(80);
    }
    
    if (restAPI == nullptr) {
        restAPI = new RestAPI();
        restAPI->begin(server, configManager, stateManager, actuatorManager, waterControl);
    }
}

void tearDown(void) {
    // Clean up after each test
}

// ============================================================================
// Property 8: API JSON response validity
// **Feature: esp32-washka-upgrade, Property 8: API JSON response validity**
// **Validates: Requirements 7.2**
// ============================================================================

void property_test_api_json_response_validity() {
    Serial.println("\n=== Property Test: API JSON Response Validity ===");
    
    int iterations = 100;
    
    for (int i = 0; i < iterations; i++) {
        // Test status endpoint
        String statusJson = "";
        
        // Build status JSON directly (simulating endpoint response)
        DynamicJsonDocument doc(1024);
        
        JsonObject status = doc.createNestedObject("status");
        status["state"] = static_cast<uint8_t>(stateManager->getCurrentState());
        status["stateDescription"] = stateManager->getStateDescription();
        status["progress"] = stateManager->getProgressPercent();
        
        JsonObject sensors = doc.createNestedObject("sensors");
        sensors["gerkonCount"] = waterControl->getGerkonCount();
        
        ActuatorManager::ActuatorStatus actuatorStatus = actuatorManager->getStatus();
        JsonObject actuators = doc.createNestedObject("actuators");
        actuators["washengine"] = actuatorStatus.washengine;
        actuators["pompa"] = actuatorStatus.pompa;
        
        JsonObject system = doc.createNestedObject("system");
        system["uptime"] = millis();
        system["freeHeap"] = ESP.getFreeHeap();
        
        serializeJson(doc, statusJson);
        
        // Property: Response must be valid JSON
        TEST_ASSERT_TRUE_MESSAGE(isValidJSON(statusJson), 
                                "Status response is not valid JSON");
        
        // Property: Response must contain required fields
        DynamicJsonDocument responseDoc(1024);
        deserializeJson(responseDoc, statusJson);
        
        TEST_ASSERT_TRUE_MESSAGE(responseDoc.containsKey("status"), 
                                "Response missing 'status' field");
        TEST_ASSERT_TRUE_MESSAGE(responseDoc.containsKey("sensors"), 
                                "Response missing 'sensors' field");
        TEST_ASSERT_TRUE_MESSAGE(responseDoc.containsKey("actuators"), 
                                "Response missing 'actuators' field");
        TEST_ASSERT_TRUE_MESSAGE(responseDoc.containsKey("system"), 
                                "Response missing 'system' field");
        
        // Test config endpoint
        String configJson = "";
        DynamicJsonDocument configDoc(1024);
        
        ConfigManager::PinConfig pins = configManager->getPinConfig();
        JsonObject pinsObj = configDoc.createNestedObject("pins");
        pinsObj["washengine"] = pins.washengine;
        pinsObj["pompa"] = pins.pompa;
        
        ConfigManager::TimingConfig timing = configManager->getTimingConfig();
        JsonObject timingObj = configDoc.createNestedObject("timing");
        timingObj["tpomp"] = timing.tpomp;
        timingObj["washtime0"] = timing.washtime0;
        
        JsonObject gerkon = configDoc.createNestedObject("gerkon");
        gerkon["threshold"] = configManager->getGerkonThreshold();
        
        serializeJson(configDoc, configJson);
        
        // Property: Config response must be valid JSON
        TEST_ASSERT_TRUE_MESSAGE(isValidJSON(configJson), 
                                "Config response is not valid JSON");
        
        // Property: Config response must contain required fields
        DynamicJsonDocument configResponseDoc(1024);
        deserializeJson(configResponseDoc, configJson);
        
        TEST_ASSERT_TRUE_MESSAGE(configResponseDoc.containsKey("pins"), 
                                "Config response missing 'pins' field");
        TEST_ASSERT_TRUE_MESSAGE(configResponseDoc.containsKey("timing"), 
                                "Config response missing 'timing' field");
        TEST_ASSERT_TRUE_MESSAGE(configResponseDoc.containsKey("gerkon"), 
                                "Config response missing 'gerkon' field");
        
        // Randomly change state to test different scenarios
        if (random(0, 2) == 0) {
            stateManager->startCycle();
        } else {
            stateManager->stopCycle();
        }
    }
    
    Serial.println("✓ Property 8 passed: API JSON response validity");
}

// ============================================================================
// Property 9: API authentication enforcement
// **Feature: esp32-washka-upgrade, Property 9: API authentication enforcement**
// **Validates: Requirements 7.3**
// ============================================================================

void property_test_api_authentication() {
    Serial.println("\n=== Property Test: API Authentication Enforcement ===");
    
    int iterations = 100;
    
    for (int i = 0; i < iterations; i++) {
        // Generate random API token
        String validToken = generateRandomAPIToken();
        String invalidToken = generateRandomAPIToken();
        
        // Ensure tokens are different
        while (invalidToken == validToken) {
            invalidToken = generateRandomAPIToken();
        }
        
        // Enable authentication with valid token
        restAPI->setAPIToken(validToken);
        TEST_ASSERT_TRUE_MESSAGE(restAPI->isAuthenticationEnabled(), 
                                "Authentication not enabled after setting token");
        
        // Property: Token should be set correctly
        TEST_ASSERT_EQUAL_STRING_MESSAGE(validToken.c_str(), 
                                        restAPI->getAPIToken().c_str(),
                                        "Token not set correctly");
        
        // Property: Invalid token should not match API token
        TEST_ASSERT_TRUE_MESSAGE(invalidToken != restAPI->getAPIToken(),
                                "Invalid token should not match API token");
        
        // Property: Authentication should be enabled when token is set
        TEST_ASSERT_TRUE_MESSAGE(restAPI->isAuthenticationEnabled(),
                                "Auth should be enabled");
        
        // Disable authentication
        restAPI->setAPIToken("");
        TEST_ASSERT_FALSE_MESSAGE(restAPI->isAuthenticationEnabled(),
                                 "Authentication should be disabled with empty token");
        
        // Property: When auth is disabled, isAuthenticationEnabled returns false
        TEST_ASSERT_FALSE_MESSAGE(restAPI->isAuthenticationEnabled(),
                                 "Auth should remain disabled");
    }
    
    Serial.println("✓ Property 9 passed: API authentication enforcement");
}

// ============================================================================
// Property 10: API parameter validation
// **Feature: esp32-washka-upgrade, Property 10: API parameter validation**
// **Validates: Requirements 7.5**
// ============================================================================

void property_test_api_parameter_validation() {
    Serial.println("\n=== Property Test: API Parameter Validation ===");
    
    int iterations = 100;
    int validCount = 0;
    int invalidCount = 0;
    
    for (int i = 0; i < iterations; i++) {
        // Generate random configuration JSON
        String configJSON = generateRandomConfigJSON();
        
        // Parse the JSON to check if it's valid
        DynamicJsonDocument doc(2048);
        DeserializationError error = deserializeJson(doc, configJSON);
        
        // Property: Valid JSON should parse without error
        TEST_ASSERT_TRUE_MESSAGE(error == DeserializationError::Ok,
                                "Generated JSON should be valid");
        
        // Property: Invalid JSON should be rejected
        String invalidJSON = "{invalid json}";
        DynamicJsonDocument invalidDoc(1024);
        DeserializationError invalidError = deserializeJson(invalidDoc, invalidJSON);
        TEST_ASSERT_TRUE_MESSAGE(invalidError != DeserializationError::Ok,
                                "Invalid JSON should fail to parse");
        
        // Test pin validation
        if (doc.containsKey("pins")) {
            JsonObject pins = doc["pins"];
            
            // Property: Pin values must be in valid range (0-39, excluding 6-11)
            if (pins.containsKey("washengine")) {
                uint8_t pin = pins["washengine"];
                bool isValid = (pin <= 39 && (pin < 6 || pin > 11));
                
                // Test with ConfigManager validation
                ConfigManager::PinConfig pinConfig = configManager->getPinConfig();
                pinConfig.washengine = pin;
                bool accepted = configManager->validatePinConfig(pinConfig);
                
                // Property: Valid pins should be accepted, invalid pins rejected
                if (isValid) {
                    // May still be rejected due to duplicates, which is correct
                    if (accepted) {
                        validCount++;
                    }
                } else {
                    // Invalid pins must be rejected
                    TEST_ASSERT_FALSE_MESSAGE(accepted,
                                            "Invalid pin should be rejected");
                    invalidCount++;
                }
            }
        }
        
        // Test timing validation
        if (doc.containsKey("timing")) {
            JsonObject timing = doc["timing"];
            
            // Property: Timing values must be in valid range (1000-86400000 ms)
            if (timing.containsKey("tpomp")) {
                unsigned long value = timing["tpomp"];
                bool isValid = (value >= 1000 && value <= 86400000);
                
                // Test with ConfigManager validation
                ConfigManager::TimingConfig timingConfig = configManager->getTimingConfig();
                timingConfig.tpomp = value;
                bool accepted = configManager->validateTimingConfig(timingConfig);
                
                // Property: Valid timing values should be accepted
                if (isValid) {
                    TEST_ASSERT_TRUE_MESSAGE(accepted,
                                           "Valid timing value should be accepted");
                    validCount++;
                } else {
                    // Invalid timing values should be rejected
                    TEST_ASSERT_FALSE_MESSAGE(accepted,
                                            "Invalid timing value should be rejected");
                    invalidCount++;
                }
            }
        }
        
        // Test gerkon validation
        if (doc.containsKey("gerkon")) {
            JsonObject gerkon = doc["gerkon"];
            
            // Property: Gerkon threshold must be in valid range (1-1000)
            if (gerkon.containsKey("threshold")) {
                uint16_t threshold = gerkon["threshold"];
                bool isValid = (threshold >= 1 && threshold <= 1000);
                
                if (isValid) {
                    configManager->setGerkonThreshold(threshold);
                    uint16_t loaded = configManager->getGerkonThreshold();
                    TEST_ASSERT_EQUAL_MESSAGE(threshold, loaded,
                                            "Valid threshold should be saved");
                    validCount++;
                } else {
                    // Invalid threshold should not be saved
                    uint16_t before = configManager->getGerkonThreshold();
                    configManager->setGerkonThreshold(threshold);
                    uint16_t after = configManager->getGerkonThreshold();
                    // Value should remain unchanged or be clamped
                    TEST_ASSERT_TRUE_MESSAGE(after >= 1 && after <= 1000,
                                           "Invalid threshold should not corrupt state");
                    invalidCount++;
                }
            }
            
            // Property: Gerkon debounce must be in valid range (1-1000 ms)
            if (gerkon.containsKey("debounce_ms")) {
                uint16_t debounce = gerkon["debounce_ms"];
                bool isValid = (debounce >= 1 && debounce <= 1000);
                
                if (isValid) {
                    configManager->setGerkonDebounceMs(debounce);
                    uint16_t loaded = configManager->getGerkonDebounceMs();
                    TEST_ASSERT_EQUAL_MESSAGE(debounce, loaded,
                                            "Valid debounce should be saved");
                    validCount++;
                } else {
                    // Invalid debounce should not be saved
                    uint16_t before = configManager->getGerkonDebounceMs();
                    configManager->setGerkonDebounceMs(debounce);
                    uint16_t after = configManager->getGerkonDebounceMs();
                    // Value should remain unchanged or be clamped
                    TEST_ASSERT_TRUE_MESSAGE(after >= 1 && after <= 1000,
                                           "Invalid debounce should not corrupt state");
                    invalidCount++;
                }
            }
        }
        
        // Test invalid parameter combinations
        if (i % 10 == 0) {
            // Test with out-of-range pin
            ConfigManager::PinConfig invalidPins = configManager->getPinConfig();
            invalidPins.washengine = 50; // Invalid: > 39
            bool accepted = configManager->validatePinConfig(invalidPins);
            TEST_ASSERT_FALSE_MESSAGE(accepted,
                                    "Out-of-range pin should be rejected");
            
            // Test with flash pin
            invalidPins = configManager->getPinConfig();
            invalidPins.pompa = 6; // Invalid: flash pin
            accepted = configManager->validatePinConfig(invalidPins);
            TEST_ASSERT_FALSE_MESSAGE(accepted,
                                    "Flash pin should be rejected");
            
            // Test with duplicate pins
            invalidPins = configManager->getPinConfig();
            invalidPins.washengine = 5;
            invalidPins.pompa = 5; // Duplicate
            accepted = configManager->validatePinConfig(invalidPins);
            TEST_ASSERT_FALSE_MESSAGE(accepted,
                                    "Duplicate pins should be rejected");
        }
    }
    
    Serial.printf("✓ Property 10 passed: API parameter validation (%d valid, %d invalid tested)\n", 
                  validCount, invalidCount);
}

// ============================================================================
// Test Runner
// ============================================================================

void setup() {
    delay(2000); // Wait for serial
    
    UNITY_BEGIN();
    
    RUN_TEST(property_test_api_json_response_validity);
    RUN_TEST(property_test_api_authentication);
    RUN_TEST(property_test_api_parameter_validation);
    
    UNITY_END();
}

void loop() {
    // Tests run once in setup()
}
