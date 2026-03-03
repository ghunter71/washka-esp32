/**
 * OTAUpdater.h
 * 
 * Manages Over-The-Air (OTA) firmware updates with safety checks.
 * Integrates with AsyncElegantOTA for web-based updates.
 */

#ifndef OTA_UPDATER_H
#define OTA_UPDATER_H

#include <Arduino.h>
#include <functional>

// Forward declarations
class AsyncWebServer;
class StateManager;

class OTAUpdater {
public:
    // Progress callback type
    typedef std::function<void(uint8_t progress)> ProgressCallback;
    
    OTAUpdater();
    ~OTAUpdater();
    
    // Initialization
    bool begin(AsyncWebServer* server, StateManager* stateManager);
    
    // Safety checks
    bool canUpdate();
    
    // Update control
    void handleUpdate();
    
    // Callbacks
    void onProgress(ProgressCallback callback);
    
    // Status
    bool isUpdating();
    uint8_t getProgress();
    
private:
    AsyncWebServer* webServer;
    StateManager* stateMgr;
    ProgressCallback progressCallback;
    bool updating;
    uint8_t currentProgress;
    
    // Internal callbacks for AsyncElegantOTA
    void onUpdateStart();
    void onUpdateProgress(size_t current, size_t total);
    void onUpdateEnd(bool success);
    void onUpdateError();
};

#endif // OTA_UPDATER_H
