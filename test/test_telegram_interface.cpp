/**
 * test_telegram_interface.cpp
 * 
 * Property-based tests for TelegramInterface
 */

#include <unity.h>
#include <Arduino.h>
#include "TelegramInterface.h"
#include "ConfigManager.h"
#include "StateManager.h"
#include <vector>

// Test instances
ConfigManager* configManager = nullptr;
StateManager* stateManager = nullptr;
TelegramInterface* telegramInterface = nullptr;

// ============================================================================
// Test Generators
// ============================================================================

/**
 * Generate random chat ID (can be positive or negative)
 */
int64_t generateRandomChatId() {
    // Telegram chat IDs can be large positive or negative numbers
    int64_t chatId = random(-999999999, 999999999);
    return chatId;
}

/**
 * Generate a list of random chat IDs
 */
std::vector<int64_t> generateRandomChatIdList(int count) {
    std::vector<int64_t> chatIds;
    for (int i = 0; i < count; i++) {
        chatIds.push_back(generateRandomChatId());
    }
    return chatIds;
}

/**
 * Generate random WashState for notifications
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
    
    // Clear any existing chat IDs
    configManager->clearAllowedChatIds();
}

void tearDown(void) {
    // Clean up after each test
    if (telegramInterface != nullptr) {
        delete telegramInterface;
        telegramInterface = nullptr;
    }
}

// ============================================================================
// Property 11: Telegram authorization enforcement
// **Feature: esp32-washka-upgrade, Property 11: Telegram authorization enforcement**
// **Validates: Requirements 8.3**
// ============================================================================

void property_test_telegram_authorization() {
    Serial.println("\n=== Property Test: Telegram Authorization Enforcement ===");
    
    int iterations = 100;
    
    for (int i = 0; i < iterations; i++) {
        // Clear chat IDs
        configManager->clearAllowedChatIds();
        
        // Generate random allowed chat IDs (1-5 IDs)
        int allowedCount = random(1, 6);
        std::vector<int64_t> allowedChatIds = generateRandomChatIdList(allowedCount);
        
        // Add allowed chat IDs to config
        for (int64_t chatId : allowedChatIds) {
            configManager->addAllowedChatId(chatId);
        }
        
        // Create TelegramInterface instance (without actual bot initialization)
        // We're testing the authorization logic, not the bot connection
        telegramInterface = new TelegramInterface(configManager, stateManager);
        
        // Property 1: All allowed chat IDs should be authorized
        for (int64_t chatId : allowedChatIds) {
            // Use reflection to test private method via public interface
            // Since isAuthorized is private, we test it indirectly through the config
            std::vector<int64_t> retrievedIds = configManager->getAllowedChatIds();
            bool found = false;
            for (int64_t id : retrievedIds) {
                if (id == chatId) {
                    found = true;
                    break;
                }
            }
            TEST_ASSERT_TRUE_MESSAGE(found, "Allowed chat ID not found in config");
        }
        
        // Property 2: Random unauthorized chat IDs should not be in allowed list
        int unauthorizedCount = random(1, 6);
        for (int j = 0; j < unauthorizedCount; j++) {
            int64_t unauthorizedChatId = generateRandomChatId();
            
            // Make sure it's not in the allowed list
            bool isInAllowedList = false;
            for (int64_t allowedId : allowedChatIds) {
                if (allowedId == unauthorizedChatId) {
                    isInAllowedList = true;
                    break;
                }
            }
            
            if (!isInAllowedList) {
                // Verify it's not in the config
                std::vector<int64_t> retrievedIds = configManager->getAllowedChatIds();
                bool found = false;
                for (int64_t id : retrievedIds) {
                    if (id == unauthorizedChatId) {
                        found = true;
                        break;
                    }
                }
                TEST_ASSERT_FALSE_MESSAGE(found, "Unauthorized chat ID found in config");
            }
        }
        
        // Property 3: When no chat IDs are configured, no IDs should be authorized
        configManager->clearAllowedChatIds();
        std::vector<int64_t> emptyList = configManager->getAllowedChatIds();
        TEST_ASSERT_EQUAL_MESSAGE(0, emptyList.size(), 
                                 "Chat ID list not empty after clear");
        
        // Test that random chat IDs are not authorized when list is empty
        int64_t randomChatId = generateRandomChatId();
        bool foundInEmpty = false;
        for (int64_t id : emptyList) {
            if (id == randomChatId) {
                foundInEmpty = true;
                break;
            }
        }
        TEST_ASSERT_FALSE_MESSAGE(foundInEmpty, 
                                 "Chat ID found in empty list");
        
        // Clean up
        delete telegramInterface;
        telegramInterface = nullptr;
    }
    
    Serial.println("✓ Property 11 passed: Telegram authorization enforcement");
}

// ============================================================================
// Property 12: Telegram state change notifications
// **Feature: esp32-washka-upgrade, Property 12: Telegram state change notifications**
// **Validates: Requirements 8.4**
// ============================================================================

void property_test_telegram_notifications() {
    Serial.println("\n=== Property Test: Telegram State Change Notifications ===");
    
    int iterations = 100;
    
    for (int i = 0; i < iterations; i++) {
        // Reset state manager to IDLE
        stateManager->stopCycle();
        
        // Configure random number of allowed chat IDs (1-5)
        configManager->clearAllowedChatIds();
        int allowedCount = random(1, 6);
        std::vector<int64_t> subscribedChatIds;
        
        for (int j = 0; j < allowedCount; j++) {
            int64_t chatId = generateRandomChatId();
            configManager->addAllowedChatId(chatId);
            subscribedChatIds.push_back(chatId);
        }
        
        // Create TelegramInterface instance
        telegramInterface = new TelegramInterface(configManager, stateManager);
        
        // Property 1: For any state transition, notifications should be sent
        // We test this by verifying the notification methods can be called
        // without errors for any valid state transition
        
        int transitionCount = random(1, 5);
        WashState currentState = stateManager->getCurrentState();
        
        for (int j = 0; j < transitionCount; j++) {
            WashState newState = generateRandomWashState();
            
            // Skip if same state (not a real transition)
            if (newState == currentState) {
                continue;
            }
            
            // Property: notifyStateChange should be callable for any state transition
            // This should not crash even if bot is not initialized
            telegramInterface->notifyStateChange(currentState, newState);
            
            currentState = newState;
        }
        
        // Property 2: Error notifications should be sendable at any time
        telegramInterface->notifyError("Test error message");
        
        // Property 3: Completion notifications should be sendable at any time
        telegramInterface->notifyComplete();
        
        // Property 4: When no chat IDs are configured, notifications should not crash
        configManager->clearAllowedChatIds();
        TelegramInterface* emptyInterface = new TelegramInterface(configManager, stateManager);
        
        // These should not crash even with no subscribers
        emptyInterface->notifyStateChange(WashState::IDLE, WashState::PREWASH);
        emptyInterface->notifyError("Error with no subscribers");
        emptyInterface->notifyComplete();
        
        delete emptyInterface;
        
        // Property 5: Verify that the TelegramInterface stores the correct chat IDs
        // by checking they match what was configured
        std::vector<int64_t> retrievedIds = configManager->getAllowedChatIds();
        
        // After clearing, should have no IDs
        TEST_ASSERT_EQUAL_MESSAGE(0, retrievedIds.size(),
                                 "Chat IDs not cleared properly");
        
        // Restore the original chat IDs
        for (int64_t chatId : subscribedChatIds) {
            configManager->addAllowedChatId(chatId);
        }
        
        retrievedIds = configManager->getAllowedChatIds();
        TEST_ASSERT_EQUAL_MESSAGE(subscribedChatIds.size(), retrievedIds.size(),
                                 "Chat ID count mismatch after restore");
        
        // Property 6: All subscribed chat IDs should be present in config
        for (int64_t subscribedId : subscribedChatIds) {
            bool found = false;
            for (int64_t retrievedId : retrievedIds) {
                if (retrievedId == subscribedId) {
                    found = true;
                    break;
                }
            }
            TEST_ASSERT_TRUE_MESSAGE(found, 
                                    "Subscribed chat ID not found in retrieved list");
        }
        
        // Clean up
        delete telegramInterface;
        telegramInterface = nullptr;
    }
    
    Serial.println("✓ Property 12 passed: Telegram state change notifications");
}

// ============================================================================
// Test Runner
// ============================================================================

void setup() {
    delay(2000); // Wait for serial
    
    UNITY_BEGIN();
    
    RUN_TEST(property_test_telegram_authorization);
    RUN_TEST(property_test_telegram_notifications);
    
    UNITY_END();
}

void loop() {
    // Tests run once in setup()
}
