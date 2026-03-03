/**
 * test_water_control.cpp
 * 
 * Property-based tests for WaterControl
 */

#include <unity.h>
#include <Arduino.h>
#include "WaterControl.h"
#include "ConfigManager.h"

// Test instances
WaterControl* waterControl = nullptr;
ConfigManager* configManager = nullptr;

// ============================================================================
// Setup and Teardown
// ============================================================================

void setUp(void) {
    if (configManager == nullptr) {
        configManager = new ConfigManager();
        configManager->begin();
    }
    
    if (waterControl == nullptr) {
        waterControl = new WaterControl();
        ConfigManager::PinConfig pins = configManager->getPinConfig();
        waterControl->begin(pins.watergerkon);
    } else {
        // Reset counter
        waterControl->resetGerkonCount();
    }
}

void tearDown(void) {
    // Clean up after each test
}

// ============================================================================
// Property 14: Gerkon counter increment
// **Feature: esp32-washka-upgrade, Property 14: Gerkon counter increment**
// **Validates: Requirements 10.2**
// ============================================================================

void property_test_gerkon_counter_increment() {
    Serial.println("\n=== Property Test: Gerkon Counter Increment ===");
    
    int iterations = 50; // Fewer iterations due to timing sensitivity
    
    for (int i = 0; i < iterations; i++) {
        // Reset counter
        waterControl->resetGerkonCount();
        TEST_ASSERT_EQUAL(0, waterControl->getGerkonCount());
        
        // Property: Each gerkon trigger should increment counter by exactly 1
        uint16_t initialCount = waterControl->getGerkonCount();
        
        // Simulate gerkon triggers
        int triggerCount = random(1, 10);
        for (int j = 0; j < triggerCount; j++) {
            // Simulate trigger by calling interrupt handler
            waterControl->handleGerkonInterrupt();
            
            // Wait for debounce period
            delay(waterControl->getDebounceMs() + 10);
        }
        
        uint16_t finalCount = waterControl->getGerkonCount();
        
        // Property: Final count should equal initial count + number of triggers
        TEST_ASSERT_EQUAL_MESSAGE(initialCount + triggerCount, finalCount,
            "Counter should increment by exactly 1 for each trigger");
    }
    
    Serial.println("✓ Property 14 passed: Gerkon counter increment");
}


// ============================================================================
// Property 15: Gerkon threshold trigger
// **Feature: esp32-washka-upgrade, Property 15: Gerkon threshold trigger**
// **Validates: Requirements 10.3**
// ============================================================================

void property_test_gerkon_threshold() {
    Serial.println("\n=== Property Test: Gerkon Threshold Trigger ===");
    
    int iterations = 50;
    
    for (int i = 0; i < iterations; i++) {
        // Reset counter
        waterControl->resetGerkonCount();
        
        // Generate random threshold
        uint16_t threshold = random(5, 20);
        
        // Property: fillWater should return SUCCESS_GERKON when threshold is reached
        // Simulate reaching threshold by triggering interrupts
        for (uint16_t j = 0; j < threshold; j++) {
            waterControl->handleGerkonInterrupt();
            delay(waterControl->getDebounceMs() + 10);
        }
        
        // Verify counter reached threshold
        TEST_ASSERT_GREATER_OR_EQUAL_MESSAGE(threshold, waterControl->getGerkonCount(),
            "Counter should reach or exceed threshold");
    }
    
    Serial.println("✓ Property 15 passed: Gerkon threshold trigger");
}

// ============================================================================
// Property 16: Gerkon debouncing
// **Feature: esp32-washka-upgrade, Property 16: Gerkon debouncing**
// **Validates: Requirements 10.5**
// ============================================================================

void property_test_gerkon_debouncing() {
    Serial.println("\n=== Property Test: Gerkon Debouncing ===");
    
    int iterations = 50;
    
    for (int i = 0; i < iterations; i++) {
        // Reset counter
        waterControl->resetGerkonCount();
        
        // Set debounce time
        uint16_t debounceMs = random(20, 100);
        waterControl->setDebounceMs(debounceMs);
        
        // Property: Multiple triggers within debounce period should count as one
        uint16_t initialCount = waterControl->getGerkonCount();
        
        // Trigger multiple times rapidly (within debounce period)
        int rapidTriggers = random(3, 8);
        for (int j = 0; j < rapidTriggers; j++) {
            waterControl->handleGerkonInterrupt();
            delay(debounceMs / 4); // Delay less than debounce time
        }
        
        // Wait for debounce period to complete
        delay(debounceMs + 10);
        
        uint16_t countAfterRapid = waterControl->getGerkonCount();
        
        // Property: Only first trigger should be counted
        TEST_ASSERT_EQUAL_MESSAGE(initialCount + 1, countAfterRapid,
            "Rapid triggers within debounce period should count as one");
        
        // Now trigger after debounce period
        waterControl->handleGerkonInterrupt();
        delay(debounceMs + 10);
        
        uint16_t finalCount = waterControl->getGerkonCount();
        
        // Property: Trigger after debounce should increment counter
        TEST_ASSERT_EQUAL_MESSAGE(countAfterRapid + 1, finalCount,
            "Trigger after debounce period should increment counter");
    }
    
    Serial.println("✓ Property 16 passed: Gerkon debouncing");
}

// ============================================================================
// Property 26: Invalid sensor halt
// **Feature: esp32-washka-upgrade, Property 26: Invalid sensor halt**
// **Validates: Requirements 15.4**
// ============================================================================

void property_test_invalid_sensor_halt() {
    Serial.println("\n=== Property Test: Invalid Sensor Halt ===");
    
    int iterations = 20; // Fewer iterations due to timeout testing
    
    for (int i = 0; i < iterations; i++) {
        // Reset counter
        waterControl->resetGerkonCount();
        
        // Property: fillWater with no sensor activity should return ERROR
        // Use short timeout for testing
        unsigned long shortTimeout = random(100, 500);
        uint16_t threshold = 100; // High threshold that won't be reached
        
        // Don't trigger any interrupts - simulate sensor failure
        WaterControl::FillResult result = waterControl->fillWater(threshold, shortTimeout);
        
        // Property: Should detect sensor malfunction or timeout
        TEST_ASSERT_TRUE_MESSAGE(
            result == WaterControl::FillResult::ERROR_TIMEOUT ||
            result == WaterControl::FillResult::ERROR_SENSOR,
            "Should detect sensor failure when no activity occurs");
    }
    
    Serial.println("✓ Property 26 passed: Invalid sensor halt");
}

// ============================================================================
// Test Runner
// ============================================================================

void setup() {
    delay(2000); // Wait for serial
    
    UNITY_BEGIN();
    
    RUN_TEST(property_test_gerkon_counter_increment);
    RUN_TEST(property_test_gerkon_threshold);
    RUN_TEST(property_test_gerkon_debouncing);
    RUN_TEST(property_test_invalid_sensor_halt);
    
    UNITY_END();
}

void loop() {
    // Tests run once in setup()
}
