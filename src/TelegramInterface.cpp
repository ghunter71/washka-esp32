/**
 * TelegramInterface.cpp
 * 
 * Implementation of Telegram bot interface.
 */

#include "TelegramInterface.h"
#include <esp_task_wdt.h>

TelegramInterface::TelegramInterface(ConfigManager* config, StateManager* state) :
    configManager(config),
    stateManager(state),
    bot(nullptr),
    lastBotCheck(0),
    initialized(false) {
}

TelegramInterface::~TelegramInterface() {
    if (bot != nullptr) {
        delete bot;
        bot = nullptr;
    }
}

bool TelegramInterface::begin() {
    // Get token from config
    botToken = configManager->getTelegramToken();
    
    if (botToken.length() == 0) {
        Serial.println("WARNING: No Telegram bot token configured");
        return false;
    }
    
    // Get allowed chat IDs
    allowedChatIds = configManager->getAllowedChatIds();
    
    if (allowedChatIds.size() == 0) {
        Serial.println("WARNING: No allowed Telegram chat IDs configured");
        // Still initialize bot, but commands will be rejected
    }
    
    // Configure secure client
    client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
    
    // Create bot instance
    bot = new UniversalTelegramBot(botToken, client);
    
    // Bot is initialized (actual connection will be tested on first message)
    Serial.println("✓ Telegram bot initialized");
    initialized = true;
    return true;
}

// ============================================================================
// Main Loop
// ============================================================================

void TelegramInterface::loop() {
    if (!initialized || bot == nullptr) {
        return;
    }
    
    unsigned long currentTime = millis();
    
    // Check for new messages at regular intervals
    if (currentTime - lastBotCheck > BOT_CHECK_INTERVAL) {
        lastBotCheck = currentTime;
        
        // Reset watchdog before potentially blocking operation
        esp_task_wdt_reset();
        
        int numNewMessages = bot->getUpdates(bot->last_message_received + 1);
        
        // Reset watchdog after blocking operation
        esp_task_wdt_reset();
        
        if (numNewMessages > 0) {
            handleNewMessages(numNewMessages);
        }
    }
}

// ============================================================================
// Message Handling
// ============================================================================

void TelegramInterface::handleNewMessages(int numNewMessages) {
    for (int i = 0; i < numNewMessages; i++) {
        String chatId = String(bot->messages[i].chat_id);
        String text = bot->messages[i].text;
        
        Serial.printf("Telegram message from %s: %s\n", chatId.c_str(), text.c_str());
        
        handleMessage(text, chatId);
    }
}

void TelegramInterface::handleMessage(const String& text, const String& chatId) {
    // Check authorization first
    if (!isAuthorized(chatId)) {
        sendMessage(chatId, "❌ Unauthorized. Your chat ID is not in the allowed list.");
        Serial.printf("Unauthorized Telegram access attempt from chat ID: %s\n", chatId.c_str());
        return;
    }
    
    // Handle commands
    if (text == "/start") {
        handleStartCommand(chatId);
    } else if (text == "/status") {
        handleStatusCommand(chatId);
    } else if (text == "/startwash") {
        handleStartWashCommand(chatId);
    } else if (text == "/stop") {
        handleStopCommand(chatId);
    } else if (text == "/config") {
        handleConfigCommand(chatId);
    } else {
        sendMessage(chatId, "Unknown command. Use /start to see available commands.");
    }
}

// ============================================================================
// Command Handlers
// ============================================================================

void TelegramInterface::handleStartCommand(const String& chatId) {
    String message = "🤖 Washka Control Bot\n\n";
    message += "Available commands:\n";
    message += "/status - Get current system status\n";
    message += "/startwash - Start wash cycle\n";
    message += "/stop - Stop wash cycle\n";
    message += "/config - Show current configuration\n";
    
    sendMessage(chatId, message);
}

void TelegramInterface::handleStatusCommand(const String& chatId) {
    String status = formatStatus();
    sendMessage(chatId, status);
}

void TelegramInterface::handleStartWashCommand(const String& chatId) {
    if (stateManager->isRunning()) {
        sendMessage(chatId, "⚠️ Wash cycle is already running.");
        return;
    }
    
    if (!stateManager->isIdle()) {
        sendMessage(chatId, "⚠️ System is not in IDLE state. Current state: " + 
                   stateManager->getStateDescription());
        return;
    }
    
    bool started = stateManager->startCycle();
    
    if (started) {
        sendMessage(chatId, "✅ Wash cycle started!");
    } else {
        sendMessage(chatId, "❌ Failed to start wash cycle.");
    }
}

void TelegramInterface::handleStopCommand(const String& chatId) {
    if (stateManager->isIdle()) {
        sendMessage(chatId, "ℹ️ System is already idle.");
        return;
    }
    
    bool stopped = stateManager->stopCycle();
    
    if (stopped) {
        sendMessage(chatId, "✅ Wash cycle stopped.");
    } else {
        sendMessage(chatId, "❌ Failed to stop wash cycle.");
    }
}

void TelegramInterface::handleConfigCommand(const String& chatId) {
    String config = formatConfig();
    sendMessage(chatId, config);
}

// ============================================================================
// Notifications
// ============================================================================

void TelegramInterface::notifyStateChange(WashState oldState, WashState newState) {
    if (!initialized || allowedChatIds.size() == 0) {
        return;
    }
    
    String message = "🔄 State changed: ";
    message += stateManager->getStateDescription(oldState);
    message += " → ";
    message += stateManager->getStateDescription(newState);
    
    // Send to all allowed chat IDs
    for (int64_t chatId : allowedChatIds) {
        sendMessage(String((long)chatId), message);
    }
}

void TelegramInterface::notifyError(const String& error) {
    if (!initialized || allowedChatIds.size() == 0) {
        return;
    }
    
    String message = "❌ ERROR: " + error;
    
    // Send to all allowed chat IDs
    for (int64_t chatId : allowedChatIds) {
        sendMessage(String((long)chatId), message);
    }
}

void TelegramInterface::notifyComplete() {
    if (!initialized || allowedChatIds.size() == 0) {
        return;
    }
    
    String message = "✅ Wash cycle complete!";
    
    // Send to all allowed chat IDs
    for (int64_t chatId : allowedChatIds) {
        sendMessage(String((long)chatId), message);
    }
}

// ============================================================================
// Authorization
// ============================================================================

bool TelegramInterface::isAuthorized(const String& chatId) {
    int64_t chatIdLong = chatId.toInt();
    return isAuthorized(chatIdLong);
}

bool TelegramInterface::isAuthorized(int64_t chatId) {
    // If no chat IDs configured, reject all
    if (allowedChatIds.size() == 0) {
        return false;
    }
    
    // Check if chat ID is in allowed list
    for (int64_t allowedId : allowedChatIds) {
        if (allowedId == chatId) {
            return true;
        }
    }
    
    return false;
}

// ============================================================================
// Helper Methods
// ============================================================================

void TelegramInterface::sendMessage(const String& chatId, const String& message) {
    if (bot != nullptr) {
        bot->sendMessage(chatId, message, "");
    }
}

String TelegramInterface::formatStatus() {
    String status = "📊 System Status\n\n";
    
    status += "State: " + stateManager->getStateDescription() + "\n";
    status += "Progress: " + String(stateManager->getProgressPercent()) + "%\n";
    
    unsigned long elapsed = stateManager->getElapsedTime();
    if (elapsed > 0) {
        unsigned long elapsedMin = elapsed / 60000;
        status += "Elapsed: " + String(elapsedMin) + " min\n";
        
        unsigned long remaining = stateManager->getEstimatedRemaining();
        unsigned long remainingMin = remaining / 60000;
        status += "Remaining: ~" + String(remainingMin) + " min\n";
    }
    
    status += "\nStatus:\n";
    status += stateManager->isIdle() ? "✓ Idle\n" : "  Idle\n";
    status += stateManager->isRunning() ? "✓ Running\n" : "  Running\n";
    status += stateManager->isPaused() ? "✓ Paused\n" : "  Paused\n";
    status += stateManager->isComplete() ? "✓ Complete\n" : "  Complete\n";
    status += stateManager->isError() ? "✓ Error\n" : "  Error\n";
    
    return status;
}

String TelegramInterface::formatConfig() {
    String config = "⚙️ Configuration\n\n";
    
    // GPIO Pins
    ConfigManager::PinConfig pins = configManager->getPinConfig();
    config += "GPIO Pins:\n";
    config += "  Wash Engine: " + String(pins.washengine) + "\n";
    config += "  Pump: " + String(pins.pompa) + "\n";
    config += "  Gerkon: " + String(pins.watergerkon) + "\n";
    config += "  Powder: " + String(pins.powder) + "\n";
    config += "  Water Valve: " + String(pins.water_valve) + "\n";
    config += "  Button: " + String(pins.button) + "\n";
    config += "  LED: " + String(pins.led) + "\n";
    
    // Timing
    ConfigManager::TimingConfig timing = configManager->getTimingConfig();
    config += "\nTiming (minutes):\n";
    config += "  Pump: " + String(timing.tpomp / 60000.0, 1) + "\n";
    config += "  Pre-wash: " + String(timing.washtime0 / 60000.0, 1) + "\n";
    config += "  Wash: " + String(timing.washtime1 / 60000.0, 1) + "\n";
    config += "  Rinse 1: " + String(timing.washtime2 / 60000.0, 1) + "\n";
    config += "  Rinse 2: " + String(timing.washtime3 / 60000.0, 1) + "\n";
    config += "  Pause: " + String(timing.pausa / 60000.0, 1) + "\n";
    config += "  Water Timeout: " + String(timing.water_in_timer / 60000.0, 1) + "\n";
    
    // Gerkon
    config += "\nGerkon:\n";
    config += "  Threshold: " + String(configManager->getGerkonThreshold()) + "\n";
    config += "  Debounce: " + String(configManager->getGerkonDebounceMs()) + " ms\n";
    
    return config;
}

// ============================================================================
// Status
// ============================================================================

bool TelegramInterface::isInitialized() {
    return initialized;
}

String TelegramInterface::getBotUsername() {
    if (bot != nullptr && initialized) {
        return "TelegramBot"; // UniversalTelegramBot doesn't provide username retrieval
    }
    return "";
}
