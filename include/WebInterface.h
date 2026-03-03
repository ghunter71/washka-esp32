/**
 * WebInterface.h
 * 
 * Serves web pages and handles WebSocket connections for real-time updates.
 */

#ifndef WEB_INTERFACE_H
#define WEB_INTERFACE_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <AsyncWebSocket.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "ConfigManager.h"
#include "StateManager.h"
#include "ActuatorManager.h"
#include "WaterControl.h"

class WebInterface {
public:
    WebInterface();
    ~WebInterface();
    
    // Initialization
    bool begin(AsyncWebServer* server, 
               ConfigManager* config,
               StateManager* state,
               ActuatorManager* actuators,
               WaterControl* water);
    
    // WebSocket broadcast
    void broadcastStatus();
    
    // Update loop (call regularly)
    void loop();
    
private:
    AsyncWebServer* webServer;
    AsyncWebSocket* ws;
    ConfigManager* configManager;
    StateManager* stateManager;
    ActuatorManager* actuatorManager;
    WaterControl* waterControl;
    
    unsigned long lastBroadcast;
    static const unsigned long BROADCAST_INTERVAL = 1000; // 1 Hz during operation
    
    // Page handlers
    void handleRoot(AsyncWebServerRequest* request);
    void handleConfigGPIO(AsyncWebServerRequest* request);
    void handleConfigTiming(AsyncWebServerRequest* request);
    void handleConfigSystem(AsyncWebServerRequest* request);
    void handleNotFound(AsyncWebServerRequest* request);
    
    // Static resource handlers
    void handleBootstrapCSS(AsyncWebServerRequest* request);
    void handleBootstrapJS(AsyncWebServerRequest* request);
    void handleStyleCSS(AsyncWebServerRequest* request);
    void handleAppJS(AsyncWebServerRequest* request);
    void handleConfigGPIOJS(AsyncWebServerRequest* request);
    void handleConfigTimingJS(AsyncWebServerRequest* request);
    void handleConfigSystemJS(AsyncWebServerRequest* request);
    
    // WebSocket handlers
    void onWebSocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                         AwsEventType type, void* arg, uint8_t* data, size_t len);
    static void webSocketEventWrapper(AsyncWebSocket* server, AsyncWebSocketClient* client,
                                      AwsEventType type, void* arg, uint8_t* data, size_t len);
    
    // Helper methods
    String buildStatusJSON();
    void sendFile(AsyncWebServerRequest* request, const char* path, const char* contentType);
    
    // Static instance for callback
    static WebInterface* instance;
};

#endif // WEB_INTERFACE_H
