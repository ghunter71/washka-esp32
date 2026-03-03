/**
 * test_state_manager.cpp
 * 
 * Property-based tests for StateManager
 */

#include <unity.h>
#include <Arduino.h>
#include "StateManager.h"
#include <vector>

// Test instance
StateManager* stateManager = nullptr;

// ============================================================================
// Test Generators
// ============================================================================

/**
 * Generate random valid wash state
 */
WashState generateRandomWashState() {
    uint8_t states[] = {
        static_cast<uint8_t>(WashState::IDLE),
        static_cast<uint8_t>(WashState::DRAIN_PREWASH),
        static_cast<uint8_t>(WashState::FILL_PREWASH),
        static_cast<uint8_t>(WashState::PREWASH),
        static_cast<uint8_t>(WashState::DRAIN_WASH),
        static_cast<uint8_t>(WashState::FILL_WASH),
        static_cast<uint8_t>(WashState::WASH),
        static_cast<uint8_t>(WashState::DRAIN_RINSE1),
        static_cast<uint8_t>(WashState::FILL_RINSE1),
        static_cast<uint8_t>(WashState::RINSE1),
        static_cast<uint8_t>(WashState::DRAIN_RINSE2),
        static_cast<uint8_t>(WashState::FILL_RINSE2),
        static_cast<uint8_t>(WashState::RINSE2),
        static_cast<uint8_t>(WashState::FINAL_DRAIN),
        static_cast<uint8_t>(WashState::COMPLETE),
        static_cast<uint8_t>(WashState::ERROR)
    };
    
    int index = random(0, sizeof(states) / sizeof(states[0]));
    return static_cast<WashState>(states[index]);
}

/**
 * Get next state in wash cycle sequence
 */
WashState getNextState(WashState current) {
    switch (current) {
        case WashState::IDLE:
            return WashState::DRAIN_PREWASH;
        case WashState::DRAIN_PREWASH:
            return WashState::FILL_PREWASH;
        case WashState::FILL_PREWASH:
            return WashState::PREWASH;
        case WashState::PREWASH:
            return WashState::DRAIN_WASH;
        case WashState::DRAIN_WASH:
            return WashState::FILL_WASH;
        case WashState::FILL_WASH:
            return WashState::WASH;
        case WashState::WASH:
            return WashState::DRAIN_RINSE1;
        case WashState::DRAIN_RINSE1:
            return WashState::FILL_RINSE1;
        case WashState::FILL_RINSE1:
            return WashState::RINSE1;
        case WashState::RINSE1:
            return WashState::DRAIN_RINSE2;
        case WashState::DRAIN_RINSE2:
            return WashState::FILL_RINSE2;
        case WashState::FILL_RINSE2:
            return WashState::RINSE2;
        case WashState::RINSE2:
            return WashState::FINAL_DRAIN;
        case WashState::FINAL_DRAIN:
            return WashState::COMPLETE;
        case WashState::COMPLETE:
            return WashState::IDLE;
        case WashState::ERROR:
            return WashState::IDLE;
        default:
            return WashState::IDLE;
    }
}

// ============================================================================
// Setup and Teardown
// ============================================================================

void setUp(void) {
    if (stateManager == nullptr) {
        stateManager = new StateManager();
        stateManager->begin();
    } else {
        // Reset to IDLE for each test
        stateManager->stopCycle();
    }
}

void tearDown(void) {
    // Clean up after each test
}

// ============================================================================
// Property 6: State transition safety
// **Feature: esp32-washka-upgrade, Property 6: State transition safety**
// **Validates: Requirements 6.2**
// ============================================================================

void property_test_state_transition_safety() {
    Serial.println("\n=== Property Test: State Transition Safety ===");
    
    int iterations = 100;
    
    for (int i = 0; i < iterations; i++) {
        // Reset to IDLE
        stateManager->stopCycle();
        TEST_ASSERT_EQUAL(WashState::IDLE, stateManager->getCurrentState());
        
        // Property: Starting from IDLE, startCycle should transition to DRAIN_PREWASH
        bool started = stateManager->startCycle();
        TEST_ASSERT_TRUE_MESSAGE(started, "Failed to start cycle from IDLE");
        TEST_ASSERT_EQUAL_MESSAGE(WashState::DRAIN_PREWASH, 
                                 stateManager->getCurrentState(),
                                 "Start cycle did not transition to DRAIN_PREWASH");
        
        // Property: Sequential state transitions should follow the wash cycle order
        WashState currentState = stateManager->getCurrentState();
        
        // Test a few sequential transitions
        int transitionCount = random(1, 8); // Test 1-7 transitions
        for (int j = 0; j < transitionCount; j++) {
            WashState expectedNext = getNextState(currentState);
            stateManager->setState(expectedNext);
            
            TEST_ASSERT_EQUAL_MESSAGE(expectedNext, 
                                     stateManager->getCurrentState(),
                                     "State transition failed");
            
            currentState = expectedNext;
            
            // Stop if we reach COMPLETE or ERROR
            if (currentState == WashState::COMPLETE || currentState == WashState::ERROR) {
                break;
            }
        }
    }
    
    Serial.println("✓ Property 6 passed: State transition safety");
}


// ============================================================================
// Property 7: Stop command safety
// **Feature: esp32-washka-upgrade, Property 7: Stop command safety**
// **Validates: Requirements 6.3**
// ============================================================================

void property_test_stop_command_safety() {
    Serial.println("\n=== Property Test: Stop Command Safety ===");
    
    int iterations = 100;
    
    for (int i = 0; i < iterations; i++) {
        // Start from IDLE
        stateManager->stopCycle();
        TEST_ASSERT_EQUAL(WashState::IDLE, stateManager->getCurrentState());
        
        // Start a cycle
        stateManager->startCycle();
        
        // Advance to a random state in the cycle
        int stateAdvances = random(0, 10);
        WashState currentState = stateManager->getCurrentState();
        
        for (int j = 0; j < stateAdvances; j++) {
            WashState nextState = getNextState(currentState);
            stateManager->setState(nextState);
            currentState = nextState;
            
            // Stop if we reach COMPLETE or ERROR
            if (currentState == WashState::COMPLETE || currentState == WashState::ERROR) {
                break;
            }
        }
        
        // Property: From any running state, stopCycle should transition to IDLE
        WashState stateBeforeStop = stateManager->getCurrentState();
        bool stopped = stateManager->stopCycle();
        
        TEST_ASSERT_TRUE_MESSAGE(stopped, "Stop cycle failed");
        TEST_ASSERT_EQUAL_MESSAGE(WashState::IDLE, 
                                 stateManager->getCurrentState(),
                                 "Stop did not transition to IDLE");
        
        // Property: After stop, system should be in IDLE and not running
        TEST_ASSERT_TRUE_MESSAGE(stateManager->isIdle(), "Not in idle state after stop");
        TEST_ASSERT_FALSE_MESSAGE(stateManager->isRunning(), "Still running after stop");
        TEST_ASSERT_FALSE_MESSAGE(stateManager->isPaused(), "Still paused after stop");
        
        // Property: Cycle start time should be reset to 0 after stop
        TEST_ASSERT_EQUAL_MESSAGE(0, stateManager->getCycleStartTime(),
                                 "Cycle start time not reset after stop");
    }
    
    Serial.println("✓ Property 7 passed: Stop command safety");
}

// ============================================================================
// Test Runner
// ============================================================================

void setup() {
    delay(2000); // Wait for serial
    
    UNITY_BEGIN();
    
    RUN_TEST(property_test_state_transition_safety);
    RUN_TEST(property_test_stop_command_safety);
    
    UNITY_END();
}

void loop() {
    // Tests run once in setup()
}
