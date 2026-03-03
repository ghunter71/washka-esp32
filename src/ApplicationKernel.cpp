/**
 * ApplicationKernel.cpp
 * 
 * Implementation of application kernel with dependency injection.
 * 
 * @version 2.0.0
 */

#include "ApplicationKernel.h"
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
#include "AuthMiddleware.h"
#include "MQTTInterface.h"
#include "ScheduleManager.h"

#include <ESPAsyncWebServer.h>
#include <esp_task_wdt.h>
#include <memory>

// Global web server instance
static AsyncWebServer* webServer = nullptr;

ApplicationKernel& ApplicationKernel::getInstance() {
    static ApplicationKernel instance;
    return instance;
}

ApplicationKernel::ApplicationKernel() 
    : state(AppState::UNINITIALIZED),
      errorCallback(nullptr),
      stateChangeCallback(nullptr) {
}

ApplicationKernel::~ApplicationKernel() {
    shutdown();
}

// ============================================================================
// Lifecycle
// ============================================================================

bool ApplicationKernel::initialize() {
    setState(AppState::INITIALIZING);
    
    Serial.println("\n========================================");
    Serial.println("  Washka ESP32 v2.0 - Starting...");
    Serial.println("========================================\n");
    
    // Create web server first
    webServer = new AsyncWebServer(80);
    
    // Initialize components in order
    if (!initLogger()) {
        reportError("DebugLogger", "Failed to initialize", false);
        // Continue anyway - logging is not critical
    }
    
    if (!initConfig()) {
        reportError("ConfigManager", "Failed to initialize", true);
        return false;
    }
    
    if (!initState()) {
        reportError("StateManager", "Failed to initialize", true);
        return false;
    }
    
    if (!initActuators()) {
        reportError("ActuatorManager", "Failed to initialize", true);
        return false;
    }
    
    if (!initWater()) {
        reportError("WaterControl", "Failed to initialize", false);
        // Continue with warning
    }
    
    if (!initAuth()) {
        reportError("AuthMiddleware", "Failed to initialize", false);
        // Continue - auth is optional
    }
    
    if (!initWiFi()) {
        reportError("WiFiManager", "Failed to connect", false);
        // Continue in AP mode
    }
    
    if (!initWeb()) {
        reportError("WebInterface", "Failed to initialize", false);
    }
    
    if (!initAPI()) {
        reportError("RestAPI", "Failed to initialize", false);
    }
    
    if (!initTelegram()) {
        // Not critical - just log warning
        if (debugLogger) {
            debugLogger->warning("Telegram not initialized");
        }
    }
    
    if (!initOTA()) {
        reportError("OTAUpdater", "Failed to initialize", false);
    }
    
    if (!initCycleController()) {
        reportError("WashCycleController", "Failed to initialize", true);
        return false;
    }
    
    if (!initMQTT()) {
        // Optional - continue without MQTT
    }
    
    if (!initScheduler()) {
        // Optional - continue without scheduler
    }
    
    // Start web server
    webServer->begin();
    Serial.println("✓ Web server started");
    
    setState(AppState::RUNNING);
    Serial.println("\n✓ System ready!\n");
    
    return true;
}

void ApplicationKernel::run() {
    if (state != AppState::RUNNING && state != AppState::SAFE_MODE) {
        return;
    }
    
    // Feed watchdog
    handleWatchdog();
    
    // Process state callbacks
    if (stateManager) {
        stateManager->processCallbacks();
    }
    
    // Handle button
    handleButton();
    
    // Handle LED indication
    handleLED();
    
    // Update components
    if (wifiManager) wifiManager->loop();
    if (webInterface) webInterface->loop();
    if (telegramInterface && configManager && configManager->isTelegramEnabled()) {
        telegramInterface->loop();
    }
    if (cycleController) cycleController->loop();
    if (mqttInterface) mqttInterface->loop();
    if (scheduleManager) scheduleManager->loop();
    
    // Handle auto-reboot
    handleAutoReboot();
    
    delay(10);  // Small delay to prevent watchdog starvation
}

void ApplicationKernel::shutdown() {
    Serial.println("Shutting down...");
    
    // Save state before shutdown
    if (stateManager) {
        stateManager->saveStateToNVS();
    }
    
    // Stop actuators safely
    if (actuatorManager) {
        actuatorManager->emergencyStop();
    }
    
    // Disconnect MQTT
    if (mqttInterface) {
        mqttInterface->disconnect();
    }
    
    // Close web server
    if (webServer) {
        delete webServer;
        webServer = nullptr;
    }
    
    setState(AppState::UNINITIALIZED);
}

// ============================================================================
// Initialization Helpers
// ============================================================================

bool ApplicationKernel::initLogger() {
    debugLogger = std::make_unique<DebugLogger>(DebugLogger::getInstance());
    if (!debugLogger->begin(webServer)) {
        Serial.println("⚠ DebugLogger init failed");
        return false;
    }
    Serial.println("✓ DebugLogger initialized");
    return true;
}

bool ApplicationKernel::initConfig() {
    configManager = std::make_unique<ConfigManager>();
    if (!configManager->begin()) {
        Serial.println("✗ ConfigManager init failed");
        return false;
    }
    Serial.println("✓ ConfigManager initialized");
    return true;
}

bool ApplicationKernel::initState() {
    stateManager = std::make_unique<StateManager>();
    if (!stateManager->begin()) {
        Serial.println("✗ StateManager init failed");
        return false;
    }
    Serial.println("✓ StateManager initialized");
    return true;
}

bool ApplicationKernel::initActuators() {
    actuatorManager = std::make_unique<ActuatorManager>();
    ConfigManager::PinConfig pins = configManager->getPinConfig();
    if (!actuatorManager->begin(pins)) {
        Serial.println("✗ ActuatorManager init failed");
        return false;
    }
    
    // Initialize button pin
    pinMode(pins.button, INPUT_PULLUP);
    
    Serial.println("✓ ActuatorManager initialized");
    return true;
}

bool ApplicationKernel::initWater() {
    waterControl = std::make_unique<WaterControl>();
    ConfigManager::PinConfig pins = configManager->getPinConfig();
    if (!waterControl->begin(pins.watergerkon)) {
        Serial.println("⚠ WaterControl init failed");
        return false;
    }
    waterControl->setDebounceMs(configManager->getGerkonDebounceMs());
    Serial.println("✓ WaterControl initialized");
    return true;
}

bool ApplicationKernel::initWiFi() {
    wifiManager = std::make_unique<WiFiManager>();
    if (!wifiManager->begin(configManager.get(), webServer)) {
        Serial.println("⚠ WiFi connection failed, running in AP mode");
        return false;
    }
    Serial.printf("✓ WiFi connected: %s\n", wifiManager->getIPAddress().c_str());
    return true;
}

bool ApplicationKernel::initWeb() {
    webInterface = std::make_unique<WebInterface>();
    if (!webInterface->begin(webServer, configManager.get(), stateManager.get(), 
                            actuatorManager.get(), waterControl.get())) {
        Serial.println("⚠ WebInterface init failed");
        return false;
    }
    Serial.println("✓ WebInterface initialized");
    return true;
}

bool ApplicationKernel::initAPI() {
    restAPI = std::make_unique<RestAPI>();
    if (!restAPI->begin(webServer, configManager.get(), stateManager.get(), 
                       actuatorManager.get(), waterControl.get())) {
        Serial.println("⚠ RestAPI init failed");
        return false;
    }
    
    // Set authentication middleware if available
    if (authMiddleware && authMiddleware->isEnabled()) {
        // restAPI->setAuthMiddleware(authMiddleware.get());
    }
    
    Serial.println("✓ RestAPI initialized");
    return true;
}

bool ApplicationKernel::initTelegram() {
    if (!configManager->isTelegramEnabled()) {
        Serial.println("ℹ Telegram disabled in config");
        return true;  // Not an error
    }
    
    telegramInterface = std::make_unique<TelegramInterface>(configManager.get(), stateManager.get());
    if (!telegramInterface->begin()) {
        Serial.println("⚠ Telegram init failed");
        return false;
    }
    Serial.println("✓ Telegram initialized");
    return true;
}

bool ApplicationKernel::initOTA() {
    otaUpdater = std::make_unique<OTAUpdater>();
    if (!otaUpdater->begin(webServer, stateManager.get())) {
        Serial.println("⚠ OTAUpdater init failed");
        return false;
    }
    
    otaUpdater->onProgress([](uint8_t progress) {
        Serial.printf("OTA Progress: %d%%\n", progress);
    });
    
    filesystemOTA = std::make_unique<FilesystemOTA>(webServer, debugLogger.get());
    filesystemOTA->begin();
    
    Serial.println("✓ OTA initialized");
    return true;
}

bool ApplicationKernel::initCycleController() {
    cycleController = std::make_unique<WashCycleController>(
        stateManager.get(), actuatorManager.get(), waterControl.get(), configManager.get());
    if (!cycleController->begin()) {
        Serial.println("✗ WashCycleController init failed");
        return false;
    }
    Serial.println("✓ WashCycleController initialized");
    return true;
}

bool ApplicationKernel::initAuth() {
    authMiddleware = std::make_unique<AuthMiddleware>();
    if (!authMiddleware->begin()) {
        Serial.println("⚠ AuthMiddleware init failed");
        return false;
    }
    
    // Set up auth failure logging
    authMiddleware->onAuthFailed([this](const String& ip, const String& reason) {
        if (debugLogger) {
            debugLogger->warning("Auth failed from " + ip + ": " + reason);
        }
    });
    
    Serial.printf("✓ AuthMiddleware initialized (auth %s)\n", 
                  authMiddleware->isEnabled() ? "ENABLED" : "disabled");
    return true;
}

bool ApplicationKernel::initMQTT() {
    mqttInterface = std::make_unique<MQTTInterface>();
    if (!mqttInterface->begin(configManager.get(), stateManager.get(), 
                              actuatorManager.get(), waterControl.get())) {
        Serial.println("⚠ MQTTInterface init failed");
        return false;
    }
    
    // Set up MQTT command handlers
    mqttInterface->onStartCommand([this]() {
        if (stateManager) {
            stateManager->startCycle();
        }
    });
    
    mqttInterface->onStopCommand([this]() {
        if (stateManager) {
            stateManager->stopCycle();
        }
    });
    
    mqttInterface->onPauseCommand([this]() {
        if (stateManager) {
            stateManager->pauseCycle();
        }
    });
    
    mqttInterface->onResumeCommand([this]() {
        if (stateManager) {
            stateManager->resumeCycle();
        }
    });
    
    Serial.println("✓ MQTTInterface initialized");
    return true;
}

bool ApplicationKernel::initScheduler() {
    scheduleManager = std::make_unique<ScheduleManager>();
    if (!scheduleManager->begin(configManager.get(), stateManager.get())) {
        Serial.println("⚠ ScheduleManager init failed");
        return false;
    }
    
    // Set up schedule trigger handler
    scheduleManager->onScheduleTrigger([this](const ScheduleManager::ScheduleEntry& schedule) {
        if (stateManager && stateManager->isIdle()) {
            stateManager->startCycle();
            if (debugLogger) {
                debugLogger->info("Cycle started by schedule: " + String(schedule.id));
            }
        }
    });
    
    Serial.println("✓ ScheduleManager initialized");
    return true;
}

// ============================================================================
// State Management
// ============================================================================

void ApplicationKernel::setState(AppState newState) {
    if (state == newState) return;
    
    AppState oldState = state;
    state = newState;
    
    if (stateChangeCallback) {
        stateChangeCallback(oldState, newState);
    }
}

// ============================================================================
// Error Handling
// ============================================================================

void ApplicationKernel::reportError(const String& component, const String& message, bool critical) {
    lastError.component = component;
    lastError.message = message;
    lastError.timestamp = millis();
    lastError.critical = critical;
    
    Serial.printf("ERROR [%s]: %s%s\n", 
                  component.c_str(), 
                  message.c_str(),
                  critical ? " (CRITICAL)" : "");
    
    if (debugLogger) {
        debugLogger->error("[" + component + "] " + message);
    }
    
    if (errorCallback) {
        errorCallback(lastError);
    }
    
    if (critical) {
        enterSafeMode();
    }
}

void ApplicationKernel::clearError() {
    lastError = ErrorInfo();
}

void ApplicationKernel::enterSafeMode() {
    Serial.println("⚠ Entering SAFE MODE");
    
    // Stop all actuators
    if (actuatorManager) {
        actuatorManager->emergencyStop();
    }
    
    // Set error state
    if (stateManager) {
        stateManager->setState(WashState::ERROR);
    }
    
    setState(AppState::SAFE_MODE);
}

void ApplicationKernel::exitSafeMode() {
    if (state != AppState::SAFE_MODE) return;
    
    Serial.println("✓ Exiting SAFE MODE");
    clearError();
    
    if (stateManager) {
        stateManager->stopCycle();
    }
    
    setState(AppState::RUNNING);
}

// ============================================================================
// Callbacks
// ============================================================================

void ApplicationKernel::onError(std::function<void(const ErrorInfo&)> callback) {
    errorCallback = callback;
}

void ApplicationKernel::onStateChange(std::function<void(AppState, AppState)> callback) {
    stateChangeCallback = callback;
}

// ============================================================================
// Main Loop Handlers
// ============================================================================

void ApplicationKernel::handleButton() {
    static unsigned long lastButtonCheck = 0;
    static bool lastButtonState = HIGH;
    static unsigned long buttonPressTime = 0;
    const unsigned long BUTTON_DEBOUNCE = 50;
    const unsigned long BUTTON_LONG_PRESS = 3000;
    
    unsigned long now = millis();
    
    if (now - lastButtonCheck < BUTTON_DEBOUNCE) return;
    lastButtonCheck = now;
    
    if (!configManager || !actuatorManager || !stateManager) return;
    
    ConfigManager::PinConfig pins = configManager->getPinConfig();
    bool currentButtonState = digitalRead(pins.button);
    
    // Falling edge (button pressed)
    if (currentButtonState == LOW && lastButtonState == HIGH) {
        buttonPressTime = now;
        
        if (stateManager->isIdle()) {
            stateManager->startCycle();
        } else if (stateManager->isPaused()) {
            stateManager->resumeCycle();
        } else if (stateManager->isRunning()) {
            stateManager->pauseCycle();
        }
    }
    // Rising edge (button released)
    else if (currentButtonState == HIGH && lastButtonState == LOW) {
        unsigned long pressDuration = now - buttonPressTime;
        
        if (pressDuration >= BUTTON_LONG_PRESS && !stateManager->isIdle()) {
            stateManager->stopCycle();
        }
    }
    
    lastButtonState = currentButtonState;
}

void ApplicationKernel::handleLED() {
    static unsigned long lastLedToggle = 0;
    static bool ledState = false;
    
    unsigned long now = millis();
    unsigned long blinkInterval = stateManager && stateManager->isRunning() ? 166 : 1000;
    
    if (now - lastLedToggle >= blinkInterval) {
        lastLedToggle = now;
        ledState = !ledState;
        
        if (actuatorManager) {
            actuatorManager->setLED(ledState);
        }
    }
}

void ApplicationKernel::handleWatchdog() {
    esp_task_wdt_reset();
}

void ApplicationKernel::handleAutoReboot() {
    static unsigned long lastRebootCheck = 0;
    const unsigned long REBOOT_INTERVAL = 30UL * 60UL * 1000UL;  // 30 minutes
    
    unsigned long now = millis();
    
    if (now - lastRebootCheck >= REBOOT_INTERVAL) {
        lastRebootCheck = now;
        
        if (stateManager && stateManager->isIdle()) {
            Serial.println("Auto-reboot: 30 minutes uptime, system idle");
            shutdown();
            ESP.restart();
        }
    }
}
