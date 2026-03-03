/**
 * ApplicationKernel.h
 * 
 * Application kernel with dependency injection pattern.
 * Manages component lifecycle and dependencies.
 * 
 * @version 2.0.0
 */

#ifndef APPLICATION_KERNEL_H
#define APPLICATION_KERNEL_H

#include <Arduino.h>
#include <memory>
#include <functional>

// Forward declarations
class ConfigManager;
class StateManager;
class ActuatorManager;
class WaterControl;
class WiFiManager;
class WebInterface;
class RestAPI;
class TelegramInterface;
class DebugLogger;
class OTAUpdater;
class WashCycleController;
class FilesystemOTA;
class AuthMiddleware;
class MQTTInterface;
class ScheduleManager;

class ApplicationKernel {
public:
    // Application state
    enum class AppState {
        UNINITIALIZED,
        INITIALIZING,
        RUNNING,
        ERROR_STATE,
        SAFE_MODE
    };
    
    // Error info
    struct ErrorInfo {
        String component;
        String message;
        unsigned long timestamp;
        bool critical;
    };
    
    // Singleton instance
    static ApplicationKernel& getInstance();
    
    // Prevent copying
    ApplicationKernel(const ApplicationKernel&) = delete;
    ApplicationKernel& operator=(const ApplicationKernel&) = delete;
    
    // Lifecycle
    bool initialize();
    void run();
    void shutdown();
    
    // State management
    AppState getState() const { return state; }
    bool isInError() const { return state == AppState::ERROR_STATE; }
    bool isInSafeMode() const { return state == AppState::SAFE_MODE; }
    
    // Component access (dependency injection)
    ConfigManager* getConfigManager() { return configManager.get(); }
    StateManager* getStateManager() { return stateManager.get(); }
    ActuatorManager* getActuatorManager() { return actuatorManager.get(); }
    WaterControl* getWaterControl() { return waterControl.get(); }
    WiFiManager* getWiFiManager() { return wifiManager.get(); }
    WebInterface* getWebInterface() { return webInterface.get(); }
    RestAPI* getRestAPI() { return restAPI.get(); }
    TelegramInterface* getTelegramInterface() { return telegramInterface.get(); }
    DebugLogger* getDebugLogger() { return debugLogger.get(); }
    OTAUpdater* getOTAUpdater() { return otaUpdater.get(); }
    WashCycleController* getWashCycleController() { return cycleController.get(); }
    FilesystemOTA* getFilesystemOTA() { return filesystemOTA.get(); }
    AuthMiddleware* getAuthMiddleware() { return authMiddleware.get(); }
    MQTTInterface* getMQTTInterface() { return mqttInterface.get(); }
    ScheduleManager* getScheduleManager() { return scheduleManager.get(); }
    
    // Error handling
    void reportError(const String& component, const String& message, bool critical = false);
    ErrorInfo getLastError() const { return lastError; }
    void clearError();
    
    // Safe mode
    void enterSafeMode();
    void exitSafeMode();
    
    // Callbacks
    void onError(std::function<void(const ErrorInfo&)> callback);
    void onStateChange(std::function<void(AppState, AppState)> callback);
    
private:
    ApplicationKernel();
    ~ApplicationKernel();
    
    // Components (unique_ptr for explicit lifetime management)
    std::unique_ptr<ConfigManager> configManager;
    std::unique_ptr<StateManager> stateManager;
    std::unique_ptr<ActuatorManager> actuatorManager;
    std::unique_ptr<WaterControl> waterControl;
    std::unique_ptr<WiFiManager> wifiManager;
    std::unique_ptr<WebInterface> webInterface;
    std::unique_ptr<RestAPI> restAPI;
    std::unique_ptr<TelegramInterface> telegramInterface;
    std::unique_ptr<DebugLogger> debugLogger;
    std::unique_ptr<OTAUpdater> otaUpdater;
    std::unique_ptr<WashCycleController> cycleController;
    std::unique_ptr<FilesystemOTA> filesystemOTA;
    std::unique_ptr<AuthMiddleware> authMiddleware;
    std::unique_ptr<MQTTInterface> mqttInterface;
    std::unique_ptr<ScheduleManager> scheduleManager;
    
    // State
    AppState state;
    ErrorInfo lastError;
    
    // Callbacks
    std::function<void(const ErrorInfo&)> errorCallback;
    std::function<void(AppState, AppState)> stateChangeCallback;
    
    // Initialization helpers
    bool initLogger();
    bool initConfig();
    bool initState();
    bool initActuators();
    bool initWater();
    bool initWiFi();
    bool initWeb();
    bool initAPI();
    bool initTelegram();
    bool initOTA();
    bool initCycleController();
    bool initAuth();
    bool initMQTT();
    bool initScheduler();
    
    // State transition
    void setState(AppState newState);
    
    // Main loop handlers
    void handleButton();
    void handleLED();
    void handleWatchdog();
    void handleAutoReboot();
};

#endif // APPLICATION_KERNEL_H
