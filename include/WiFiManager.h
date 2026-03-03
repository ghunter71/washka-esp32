/**
 * WiFiManager.h
 * 
 * Manages WiFi connection with AP mode fallback and captive portal.
 */

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "ConfigManager.h"

class WiFiManager {
public:
    // Connection status enumeration
    enum class ConnectionStatus {
        CONNECTED,      // Connected to WiFi
        CONNECTING,     // Attempting to connect
        AP_MODE,        // Running in Access Point mode
        FAILED          // Connection failed
    };
    
    WiFiManager();
    ~WiFiManager();
    
    // Initialization
    bool begin(ConfigManager* config, AsyncWebServer* server);
    
    // Connection management
    bool connect();
    void disconnect();
    
    // Status
    ConnectionStatus getStatus();
    String getIPAddress();
    String getAPSSID();
    int getRSSI();
    
    // Access Point mode
    void startConfigPortal();
    void stopConfigPortal();
    bool isAPMode();
    
    // Update loop (call regularly)
    void loop();
    
private:
    ConfigManager* configManager;
    AsyncWebServer* webServer;
    ConnectionStatus status;
    String apSSID;
    bool apMode;
    unsigned long lastConnectionAttempt;
    int connectionAttempts;
    
    // Constants
    static const int MAX_CONNECTION_ATTEMPTS = 20;
    static const unsigned long CONNECTION_TIMEOUT = 500;
    static const char* AP_PASSWORD;
    
    // Helper methods
    bool attemptConnection();
    void setupAPMode();
    void setupCaptivePortal();
    void handleWiFiConfig(AsyncWebServerRequest* request, uint8_t* data, size_t len);
};

#endif // WIFI_MANAGER_H
