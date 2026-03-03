/**
 * Washka ESP32 - Dishwasher Control System
 * 
 * Main entry point for the ESP32-based dishwasher control system.
 * Manages wash cycles, web interface, REST API, and Telegram bot.
 * 
 * @version 1.0.0
 * @author Washka Team
 */

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <WebSerial.h>
#include <esp_task_wdt.h>

// Component includes
#include "ConfigManager.h"
#include "StateManager.h"
#include "ActuatorManager.h"
#include "WaterControl.h"
#include "WiFiManager.h"
#include "WebInterface.h"
#include "RestAPI.h"
#include "TelegramInterface.h"
#include "DebugLogger.h"
#include "OTAUpdater.h"
#include "WashCycleController.h"
#include "FilesystemOTA.h"

// Version information
const char* FIRMWARE_VERSION = "1.0.0";
const char* BUILD_DATE = __DATE__;
const char* BUILD_TIME = __TIME__;

// Global objects
AsyncWebServer server(80);

// Component instances
ConfigManager configManager;
StateManager stateManager;
ActuatorManager actuatorManager;
WaterControl waterControl;
WiFiManager wifiManager;
WebInterface webInterface;
RestAPI restAPI;
TelegramInterface telegramInterface(&configManager, &stateManager);
DebugLogger& logger = DebugLogger::getInstance();
OTAUpdater otaUpdater;
WashCycleController cycleController(&stateManager, &actuatorManager, &waterControl, &configManager);
FilesystemOTA filesystemOTA(&server, &logger);

/**
 * Setup function - runs once at startup
 */
void setup() {
    // Initialize serial communication
    Serial.begin(115200);
    delay(2000);  // Увеличена задержка для стабилизации
    
    Serial.println("\n\n");
    Serial.println("========================================");
    Serial.println("  Washka ESP32 - Dishwasher Control");
    Serial.println("========================================");
    Serial.printf("  Version: %s\n", FIRMWARE_VERSION);
    Serial.printf("  Build: %s %s\n", BUILD_DATE, BUILD_TIME);
    Serial.println("========================================\n");
    
    // Configure watchdog timer (60 seconds timeout for SSL operations)
    esp_task_wdt_init(60, true);  // 60 second timeout, panic on timeout
    esp_task_wdt_add(NULL);        // Add current task to WDT
    Serial.println("✓ Watchdog timer configured (60s timeout)");
    
    // Initialize DebugLogger first
    Serial.println("\nInitializing DebugLogger...");
    esp_task_wdt_reset();  // Reset watchdog
    try {
        if (!logger.begin(&server)) {
            Serial.println("ERROR: Failed to initialize DebugLogger");
        } else {
            Serial.println("✓ DebugLogger initialized");
        }
    } catch (...) {
        Serial.println("ERROR: Exception in DebugLogger initialization");
    }
    
    // Register system state dump callback
    logger.onSystemStateDump([]() -> String {
        String state = "";
        state += "State: " + stateManager.getStateDescription() + "\n";
        state += "Progress: " + String(stateManager.getProgressPercent()) + "%\n";
        state += "Elapsed: " + String(stateManager.getElapsedTime() / 1000) + "s\n";
        
        ActuatorManager::ActuatorStatus actuators = actuatorManager.getStatus();
        state += "Actuators:\n";
        state += "  Wash Engine: " + String(actuators.washengine ? "ON" : "OFF") + "\n";
        state += "  Pump: " + String(actuators.pompa ? "ON" : "OFF") + "\n";
        state += "  Water Valve: " + String(actuators.water_valve ? "ON" : "OFF") + "\n";
        state += "  Powder: " + String(actuators.powder ? "ON" : "OFF") + "\n";
        state += "  LED: " + String(actuators.led ? "ON" : "OFF") + "\n";
        
        state += "Gerkon Count: " + String(waterControl.getGerkonCount()) + "\n";
        state += "WiFi Status: " + wifiManager.getIPAddress() + "\n";
        
        return state;
    });
    
    // Initialize ConfigManager
    Serial.println("Initializing ConfigManager...");
    esp_task_wdt_reset();  // Reset watchdog
    try {
        if (!configManager.begin()) {
            Serial.println("ERROR: Failed to initialize ConfigManager");
        } else {
            Serial.println("✓ ConfigManager initialized");
        }
    } catch (...) {
        Serial.println("ERROR: Exception in ConfigManager initialization");
    }
    
    // Initialize StateManager
    Serial.println("Initializing StateManager...");
    try {
        if (!stateManager.begin()) {
            Serial.println("ERROR: Failed to initialize StateManager");
        } else {
            Serial.println("✓ StateManager initialized");
        }
    } catch (...) {
        Serial.println("ERROR: Exception in StateManager initialization");
    }
    
    // Initialize ActuatorManager
    logger.info("Initializing ActuatorManager...");
    ConfigManager::PinConfig pins = configManager.getPinConfig();
    if (!actuatorManager.begin(pins)) {
        logger.error("Failed to initialize ActuatorManager");
    }
    
    // Initialize button pin
    pinMode(pins.button, INPUT_PULLUP);
    Serial.printf("✓ Button pin %d configured (INPUT_PULLUP)\n", pins.button);
    
    // Initialize WaterControl
    logger.info("Initializing WaterControl...");
    if (!waterControl.begin(pins.watergerkon)) {
        logger.error("Failed to initialize WaterControl");
    }
    waterControl.setDebounceMs(configManager.getGerkonDebounceMs());
    
    // Setup state change callback
    stateManager.onStateChange([](WashState oldState, WashState newState) {
        logger.info("State changed: " + 
                   stateManager.getStateDescription(oldState) + " -> " +
                   stateManager.getStateDescription(newState));
        
        // Notify Telegram bot of state changes
        telegramInterface.notifyStateChange(oldState, newState);
        
        // Notify on completion
        if (newState == WashState::COMPLETE) {
            telegramInterface.notifyComplete();
            logger.info("Wash cycle completed successfully");
        }
        
        // Notify on error
        if (newState == WashState::ERROR) {
            telegramInterface.notifyError("System entered error state");
            logger.error("System entered ERROR state");
        }
    });
    
    // Initialize WiFiManager
    logger.info("Initializing WiFiManager...");
    Serial.println("Initializing WiFiManager...");
    esp_task_wdt_reset();  // Reset watchdog - WiFi может занять время
    if (!wifiManager.begin(&configManager, &server)) {
        logger.warning("WiFi connection failed, running in AP mode");
        Serial.println("⚠ WiFi connection failed, running in AP mode");
    } else {
        Serial.println("✓ WiFiManager initialized");
    }
    
    // Initialize WebInterface
    logger.info("Initializing WebInterface...");
    if (!webInterface.begin(&server, &configManager, &stateManager, &actuatorManager, &waterControl)) {
        logger.error("Failed to initialize WebInterface");
    }
    
    // Initialize RestAPI
    logger.info("Initializing RestAPI...");
    if (!restAPI.begin(&server, &configManager, &stateManager, &actuatorManager, &waterControl)) {
        logger.error("Failed to initialize RestAPI");
    }
    // Optional: Set API token for authentication
    // restAPI.setAPIToken("your-secret-token-here");
    
    // Initialize TelegramInterface
    logger.info("Initializing TelegramInterface...");
    if (configManager.isTelegramEnabled()) {
        if (!telegramInterface.begin()) {
            logger.warning("Telegram bot not configured or failed to initialize");
        } else {
            logger.info("Telegram bot initialized successfully");
        }
    } else {
        logger.info("Telegram bot disabled in configuration");
    }
    
    // Initialize OTAUpdater
    logger.info("Initializing OTAUpdater...");
    if (!otaUpdater.begin(&server, &stateManager)) {
        logger.error("Failed to initialize OTAUpdater");
    } else {
        logger.info("OTA updates available at /update");
        
        // Setup progress callback
        otaUpdater.onProgress([](uint8_t progress) {
            logger.info("OTA Progress: " + String(progress) + "%");
        });
    }
    
    // Initialize FilesystemOTA
    logger.info("Initializing FilesystemOTA...");
    filesystemOTA.begin();
    logger.info("Filesystem OTA endpoints available at /api/update/filesystem");
    
    // Initialize WashCycleController
    logger.info("Initializing WashCycleController...");
    if (!cycleController.begin()) {
        logger.error("Failed to initialize WashCycleController");
    } else {
        logger.info("WashCycleController initialized successfully");
    }
    
    // Start web server
    server.begin();
    logger.info("Web server started");
    
    logger.info("System initialization complete");
    Serial.println("\n✓ System initialization complete");
    Serial.println("System ready!\n");
    
    // Print system status
    Serial.println("System Status:");
    Serial.printf("  WiFi: %s\n", wifiManager.getStatus() == WiFiManager::ConnectionStatus::CONNECTED ? "Connected" : "AP Mode");
    Serial.printf("  IP: %s\n", wifiManager.getIPAddress().c_str());
    Serial.printf("  State: %s\n", stateManager.getStateDescription().c_str());
    Serial.println();
    
    logger.info("System ready - WiFi: " + wifiManager.getIPAddress() + 
               ", State: " + stateManager.getStateDescription());
}

/**
 * Main loop - runs continuously
 */
void loop() {
    // Feed watchdog timer
    esp_task_wdt_reset();
    
    // Auto-reboot every 30 minutes (only when idle)
    static unsigned long lastRebootCheck = 0;
    const unsigned long REBOOT_INTERVAL = 30UL * 60UL * 1000UL; // 30 minutes in milliseconds
    unsigned long currentMillis = millis();
    
    if (currentMillis - lastRebootCheck >= REBOOT_INTERVAL) {
        lastRebootCheck = currentMillis;
        
        // Only reboot if system is idle (not during wash cycle)
        if (stateManager.isIdle()) {
            Serial.println("Auto-reboot: 30 minutes elapsed, system idle - rebooting...");
            logger.info("Auto-reboot: 30 minutes uptime, restarting system");
            delay(1000);
            ESP.restart();
        } else {
            Serial.println("Auto-reboot: 30 minutes elapsed, but system is busy - skipping reboot");
            logger.info("Auto-reboot: Skipped (system not idle)");
        }
    }
    
    // Handle physical button press
    static unsigned long lastButtonCheck = 0;
    static bool lastButtonState = HIGH;
    static unsigned long buttonPressTime = 0;
    const unsigned long BUTTON_DEBOUNCE = 50;  // 50ms debounce
    const unsigned long BUTTON_LONG_PRESS = 3000;  // 3 seconds for long press
    
    if (currentMillis - lastButtonCheck >= BUTTON_DEBOUNCE) {
        lastButtonCheck = currentMillis;
        
        ConfigManager::PinConfig pins = configManager.getPinConfig();
        bool currentButtonState = digitalRead(pins.button);
        
        // Detect falling edge (HIGH -> LOW transition = button pressed)
        if (currentButtonState == LOW && lastButtonState == HIGH) {
            buttonPressTime = currentMillis;
            Serial.println("Button: Falling edge detected (pressed)");
            
            // Trigger action immediately on press (falling edge)
            if (stateManager.isIdle()) {
                stateManager.startCycle();
                logger.info("Cycle started by button");
            } else if (stateManager.isPaused()) {
                stateManager.resumeCycle();
                logger.info("Cycle resumed by button");
            } else if (stateManager.isRunning()) {
                stateManager.pauseCycle();
                logger.info("Cycle paused by button");
            }
        }
        // Detect rising edge (LOW -> HIGH transition = button released)
        else if (currentButtonState == HIGH && lastButtonState == LOW) {
            unsigned long pressDuration = currentMillis - buttonPressTime;
            
            // Check if it was a long press for STOP action
            if (pressDuration >= BUTTON_LONG_PRESS) {
                Serial.println("Button: Long press detected - STOP");
                if (!stateManager.isIdle()) {
                    stateManager.stopCycle();
                    logger.info("Cycle stopped by button (long press)");
                }
            }
        }
        
        lastButtonState = currentButtonState;
    }
    
    // Update WiFiManager (handles reconnection and captive portal)
    wifiManager.loop();
    
    // Update WebInterface (handles WebSocket and status broadcasts)
    webInterface.loop();
    
    // Update TelegramInterface (check for new messages) - only if enabled
    if (configManager.isTelegramEnabled()) {
        telegramInterface.loop();
    }
    
    // Execute wash cycle controller
    cycleController.loop();
    
    // LED status indication
    static unsigned long lastLedToggle = 0;
    static bool ledState = false;
    
    // LED blink pattern based on state
    unsigned long blinkInterval;
    if (stateManager.isIdle() || stateManager.isComplete()) {
        blinkInterval = 1000; // Slow blink: 1 second on, 1 second off
    } else {
        blinkInterval = 166;  // Fast blink: 3 times per second (166ms on/off)
    }
    
    if (currentMillis - lastLedToggle >= blinkInterval) {
        lastLedToggle = currentMillis;
        ledState = !ledState;
        actuatorManager.setLED(ledState);
    }
    
    delay(100);
}
