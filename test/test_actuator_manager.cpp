/**
 * test_actuator_manager.cpp
 * 
 * Property-based tests for ActuatorManager
 */

#include <unity.h>
#include <Arduino.h>
#include "ActuatorManager.h"
#include "ConfigManager.h"

// Test instances
ActuatorManager* actuatorManager = nullptr;
ConfigManager* configManager = nullptr;

// ============================================================================
// Setup and Teardown
// ============================================================================

void setUp(void) {
    if (configManager == nullptr) {
        configManager = new ConfigManager();
        configManager->begin();
    }
    
    if (actuatorManager == nullptr) {
        actuatorManager = new ActuatorManager();
        ConfigManager::PinConfig pins = configManager->getPinConfig();
        actuatorManager->begin(pins);
    } else {
        // Reset all actuators to OFF
        actuatorManager->emergencyStop();
    }
}

void tearDown(void) {
    // Clean up after each test
}

// ============================================================================
// Property 23: Actuator mutual exclusion
// **Feature: esp32-washka-upgrade, Property 23: Actuator mutual exclusion**
// **Validates: Requirements 15.1**
// ============================================================================

void property_test_actuator_mutual_exclusion() {
    Serial.println("\n=== Property Test: Actuator Mutual Exclusion ===");
    
    int iterations = 100;
    
    for (int i = 0; i < iterations; i++) {
        // Reset to known state
        actuatorManager->emergencyStop();
        
        // Property: Cannot turn on pump if water valve is on
        actuatorManager->setWaterValve(true);
        bool pumpResult = actuatorManager->setPump(true);
        TEST_ASSERT_FALSE_MESSAGE(pumpResult, 
            "Pump should not turn on when water valve is on");
        
        ActuatorManager::ActuatorStatus status = actuatorManager->getStatus();
        TEST_ASSERT_TRUE_MESSAGE(status.water_valve, "Water valve should be on");
        TEST_ASSERT_FALSE_MESSAGE(status.pompa, "Pump should be off (mutual exclusion)");
        
        // Reset
        actuatorManager->emergencyStop();
        
        // Property: Cannot turn on water valve if pump is on
        actuatorManager->setPump(true);
        bool valveResult = actuatorManager->setWaterValve(true);
        TEST_ASSERT_FALSE_MESSAGE(valveResult, 
            "Water valve should not turn on when pump is on");
        
        status = actuatorManager->getStatus();
        TEST_ASSERT_TRUE_MESSAGE(status.pompa, "Pump should be on");
        TEST_ASSERT_FALSE_MESSAGE(status.water_valve, 
            "Water valve should be off (mutual exclusion)");
        
        // Reset
        actuatorManager->emergencyStop();
        
        // Property: Can turn on pump when water valve is off
        actuatorManager->setWaterValve(false);
        pumpResult = actuatorManager->setPump(true);
        TEST_ASSERT_TRUE_MESSAGE(pumpResult, 
            "Pump should turn on when water valve is off");
        
        // Property: Can turn on water valve when pump is off
        actuatorManager->emergencyStop();
        actuatorManager->setPump(false);
        valveResult = actuatorManager->setWaterValve(true);
        TEST_ASSERT_TRUE_MESSAGE(valveResult, 
            "Water valve should turn on when pump is off");
    }
    
    Serial.println("✓ Property 23 passed: Actuator mutual exclusion");
}


// ============================================================================
// Property 24: Timeout safety shutdown
// **Feature: esp32-washka-upgrade, Property 24: Timeout safety shutdown**
// **Validates: Requirements 15.2**
// ============================================================================

void property_test_timeout_safety() {
    Serial.println("\n=== Property Test: Timeout Safety Shutdown ===");
    
    int iterations = 100;
    
    for (int i = 0; i < iterations; i++) {
        // Set random actuator states
        bool washState = random(0, 2);
        bool pumpState = random(0, 2);
        bool powderState = random(0, 2);
        
        actuatorManager->emergencyStop();
        actuatorManager->setWashEngine(washState);
        if (!pumpState) {
            actuatorManager->setWaterValve(random(0, 2));
        }
        if (actuatorManager->canDrain()) {
            actuatorManager->setPump(pumpState);
        }
        actuatorManager->setPowderDispenser(powderState);
        
        // Property: After emergency stop (simulating timeout), all actuators should be OFF
        actuatorManager->emergencyStop();
        
        ActuatorManager::ActuatorStatus status = actuatorManager->getStatus();
        TEST_ASSERT_FALSE_MESSAGE(status.washengine, 
            "Wash engine should be OFF after emergency stop");
        TEST_ASSERT_FALSE_MESSAGE(status.pompa, 
            "Pump should be OFF after emergency stop");
        TEST_ASSERT_FALSE_MESSAGE(status.water_valve, 
            "Water valve should be OFF after emergency stop");
        TEST_ASSERT_FALSE_MESSAGE(status.powder, 
            "Powder dispenser should be OFF after emergency stop");
    }
    
    Serial.println("✓ Property 24 passed: Timeout safety shutdown");
}

// ============================================================================
// Property 25: Emergency stop completeness
// **Feature: esp32-washka-upgrade, Property 25: Emergency stop completeness**
// **Validates: Requirements 15.3**
// ============================================================================

void property_test_emergency_stop() {
    Serial.println("\n=== Property Test: Emergency Stop Completeness ===");
    
    int iterations = 100;
    
    for (int i = 0; i < iterations; i++) {
        // Set all actuators to random states
        actuatorManager->emergencyStop();
        
        bool washState = random(0, 2);
        bool ledState = random(0, 2);
        bool powderState = random(0, 2);
        
        actuatorManager->setWashEngine(washState);
        actuatorManager->setLED(ledState);
        actuatorManager->setPowderDispenser(powderState);
        
        // Randomly set either pump or valve (but not both due to mutual exclusion)
        if (random(0, 2)) {
            actuatorManager->setPump(true);
        } else {
            actuatorManager->setWaterValve(true);
        }
        
        // Property: Emergency stop should immediately set ALL actuators to OFF
        actuatorManager->emergencyStop();
        
        ActuatorManager::ActuatorStatus status = actuatorManager->getStatus();
        TEST_ASSERT_FALSE_MESSAGE(status.washengine, 
            "All actuators should be OFF after emergency stop");
        TEST_ASSERT_FALSE_MESSAGE(status.pompa, 
            "All actuators should be OFF after emergency stop");
        TEST_ASSERT_FALSE_MESSAGE(status.water_valve, 
            "All actuators should be OFF after emergency stop");
        TEST_ASSERT_FALSE_MESSAGE(status.powder, 
            "All actuators should be OFF after emergency stop");
        TEST_ASSERT_FALSE_MESSAGE(status.led, 
            "All actuators should be OFF after emergency stop");
    }
    
    Serial.println("✓ Property 25 passed: Emergency stop completeness");
}

// ============================================================================
// Test Runner
// ============================================================================

void setup() {
    delay(2000); // Wait for serial
    
    UNITY_BEGIN();
    
    RUN_TEST(property_test_actuator_mutual_exclusion);
    RUN_TEST(property_test_timeout_safety);
    RUN_TEST(property_test_emergency_stop);
    
    UNITY_END();
}

void loop() {
    // Tests run once in setup()
}
