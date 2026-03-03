/**
 * TelegramInterface.h
 * 
 * Telegram bot interface for remote control and notifications.
 * Provides command handling and state change notifications.
 */

#ifndef TELEGRAM_INTERFACE_H
#define TELEGRAM_INTERFACE_H

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include "ConfigManager.h"
#include "StateManager.h"
#include <vector>

class TelegramInterface {
public:
    TelegramInterface(ConfigManager* config, StateManager* state);
    ~TelegramInterface();
    
    // Initialization
    bool begin();
    
    // Main loop - call regularly to check for messages
    void loop();
    
    // Notifications
    void notifyStateChange(WashState oldState, WashState newState);
    void notifyError(const String& error);
    void notifyComplete();
    
    // Status
    bool isInitialized();
    String getBotUsername();
    
private:
    ConfigManager* configManager;
    StateManager* stateManager;
    WiFiClientSecure client;
    UniversalTelegramBot* bot;
    
    unsigned long lastBotCheck;
    const unsigned long BOT_CHECK_INTERVAL = 1000; // Check every second
    
    bool initialized;
    String botToken;
    std::vector<int64_t> allowedChatIds;
    
    // Message handling
    void handleNewMessages(int numNewMessages);
    void handleMessage(const String& text, const String& chatId);
    
    // Command handlers
    void handleStartCommand(const String& chatId);
    void handleStatusCommand(const String& chatId);
    void handleStartWashCommand(const String& chatId);
    void handleStopCommand(const String& chatId);
    void handleConfigCommand(const String& chatId);
    
    // Authorization
    bool isAuthorized(const String& chatId);
    bool isAuthorized(int64_t chatId);
    
    // Helper methods
    void sendMessage(const String& chatId, const String& message);
    String formatStatus();
    String formatConfig();
};

#endif // TELEGRAM_INTERFACE_H
