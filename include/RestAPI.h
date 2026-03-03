/**
 * RestAPI.h
 * 
 * REST API for programmatic control of the wash system.
 * Provides JSON endpoints for status, configuration, and control.
 */

#ifndef REST_API_H
#define REST_API_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "ConfigManager.h"
#include "StateManager.h"
#include "ActuatorManager.h"
#include "WaterControl.h"
#include "PinLegend.h"

class RestAPI {
public:
    RestAPI();
    ~RestAPI();
    
    // Initialization
    bool begin(AsyncWebServer* server, 
               ConfigManager* config,
               StateManager* state,
               ActuatorManager* actuators,
               WaterControl* water);
    
    // API Token management
    void setAPIToken(const String& token);
    String getAPIToken();
    bool isAuthenticationEnabled();
    
private:
    AsyncWebServer* webServer;
    ConfigManager* configManager;
    StateManager* stateManager;
    ActuatorManager* actuatorManager;
    WaterControl* waterControl;
    
    String apiToken;
    bool authEnabled;
    
    // Endpoint handlers
    void handleGetStatus(AsyncWebServerRequest* request);
    void handleGetConfig(AsyncWebServerRequest* request);
    void handlePostConfig(AsyncWebServerRequest* request, uint8_t* data, size_t len);
    void handlePostWiFiConfig(AsyncWebServerRequest* request, uint8_t* data, size_t len);
    void handlePostTelegramConfig(AsyncWebServerRequest* request, uint8_t* data, size_t len);
    void handlePostTimingConfig(AsyncWebServerRequest* request, uint8_t* data, size_t len);
    void handleControlStart(AsyncWebServerRequest* request);
    void handleControlStop(AsyncWebServerRequest* request);
    void handleControlPause(AsyncWebServerRequest* request);
    void handleControlResume(AsyncWebServerRequest* request);
    void handleControlManual(AsyncWebServerRequest* request, uint8_t* data, size_t len);
    void handleDocs(AsyncWebServerRequest* request);
    void handleGetPins(AsyncWebServerRequest* request);
    void handleSystemRestart(AsyncWebServerRequest* request);
    void handleConfigExport(AsyncWebServerRequest* request);
    void handleConfigImport(AsyncWebServerRequest* request, uint8_t* data, size_t len);
    void handleFactoryReset(AsyncWebServerRequest* request);
    
    // Helper methods
    bool authenticateRequest(AsyncWebServerRequest* request);
    void sendJsonResponse(AsyncWebServerRequest* request, int code, const String& json);
    void sendErrorResponse(AsyncWebServerRequest* request, int code, const String& error);
    String buildStatusJson();
    String buildConfigJson();
    String buildOpenAPIDoc();
};

#endif // REST_API_H
