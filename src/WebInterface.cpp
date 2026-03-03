/**
 * WebInterface.cpp
 * 
 * Implementation of web interface with WebSocket support.
 */

#include "WebInterface.h"

// Static instance for callback
WebInterface* WebInterface::instance = nullptr;

WebInterface::WebInterface() :
    webServer(nullptr),
    ws(nullptr),
    configManager(nullptr),
    stateManager(nullptr),
    actuatorManager(nullptr),
    waterControl(nullptr),
    lastBroadcast(0) {
    instance = this;
}

WebInterface::~WebInterface() {
    if (ws) {
        delete ws;
    }
}

bool WebInterface::begin(AsyncWebServer* server,
                        ConfigManager* config,
                        StateManager* state,
                        ActuatorManager* actuators,
                        WaterControl* water) {
    webServer = server;
    configManager = config;
    stateManager = state;
    actuatorManager = actuators;
    waterControl = water;
    
    // Initialize LittleFS for serving static files
    if (!LittleFS.begin(true)) {
        Serial.println("ERROR: Failed to mount LittleFS");
        return false;
    }
    
    // Create WebSocket
    ws = new AsyncWebSocket("/ws");
    ws->onEvent(webSocketEventWrapper);
    webServer->addHandler(ws);
    
    // Setup route handlers for HTML pages
    webServer->on("/", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleRoot(request);
    });
    
    webServer->on("/config-gpio.html", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleConfigGPIO(request);
    });
    
    webServer->on("/config-timing.html", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleConfigTiming(request);
    });
    
    webServer->on("/config-system.html", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleConfigSystem(request);
    });
    
    // Setup route handlers for static resources
    webServer->on("/bootstrap.min.css", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleBootstrapCSS(request);
    });
    
    webServer->on("/bootstrap.bundle.min.js", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleBootstrapJS(request);
    });
    
    webServer->on("/style.css", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleStyleCSS(request);
    });
    
    webServer->on("/app.js", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleAppJS(request);
    });
    
    webServer->on("/config-gpio.js", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleConfigGPIOJS(request);
    });
    
    webServer->on("/config-timing.js", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleConfigTimingJS(request);
    });
    
    webServer->on("/config-system.js", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleConfigSystemJS(request);
    });
    
    // 404 handler
    webServer->onNotFound([this](AsyncWebServerRequest* request) {
        handleNotFound(request);
    });
    
    Serial.println("✓ WebInterface initialized");
    return true;
}

// ============================================================================
// Update Loop
// ============================================================================

void WebInterface::loop() {
    // Clean up WebSocket clients
    ws->cleanupClients();
    
    // Broadcast status updates at regular intervals
    unsigned long now = millis();
    if (now - lastBroadcast >= BROADCAST_INTERVAL) {
        lastBroadcast = now;
        broadcastStatus();
    }
}

// ============================================================================
// WebSocket
// ============================================================================

void WebInterface::broadcastStatus() {
    if (ws->count() == 0) {
        return; // No clients connected
    }
    
    String json = buildStatusJSON();
    ws->textAll(json);
}

void WebInterface::webSocketEventWrapper(AsyncWebSocket* server, AsyncWebSocketClient* client,
                                        AwsEventType type, void* arg, uint8_t* data, size_t len) {
    if (instance) {
        instance->onWebSocketEvent(server, client, type, arg, data, len);
    }
}

void WebInterface::onWebSocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                                   AwsEventType type, void* arg, uint8_t* data, size_t len) {
    switch (type) {
        case WS_EVT_CONNECT:
            Serial.printf("WebSocket client #%u connected from %s\n", 
                         client->id(), client->remoteIP().toString().c_str());
            // Send initial status to new client
            client->text(buildStatusJSON());
            break;
            
        case WS_EVT_DISCONNECT:
            Serial.printf("WebSocket client #%u disconnected\n", client->id());
            break;
            
        case WS_EVT_ERROR:
            Serial.printf("WebSocket client #%u error(%u): %s\n", 
                         client->id(), *((uint16_t*)arg), (char*)data);
            break;
            
        case WS_EVT_DATA:
            // Handle incoming WebSocket data if needed
            break;
            
        default:
            break;
    }
}

// ============================================================================
// Page Handlers
// ============================================================================

void WebInterface::handleRoot(AsyncWebServerRequest* request) {
    sendFile(request, "/index.html", "text/html");
}

void WebInterface::handleConfigGPIO(AsyncWebServerRequest* request) {
    sendFile(request, "/config-gpio.html", "text/html");
}

void WebInterface::handleConfigTiming(AsyncWebServerRequest* request) {
    sendFile(request, "/config-timing.html", "text/html");
}

void WebInterface::handleConfigSystem(AsyncWebServerRequest* request) {
    sendFile(request, "/config-system.html", "text/html");
}

void WebInterface::handleNotFound(AsyncWebServerRequest* request) {
    request->send(404, "text/plain", "Not Found");
}

// ============================================================================
// Static Resource Handlers
// ============================================================================

void WebInterface::handleBootstrapCSS(AsyncWebServerRequest* request) {
    sendFile(request, "/bootstrap.min.css", "text/css");
}

void WebInterface::handleBootstrapJS(AsyncWebServerRequest* request) {
    sendFile(request, "/bootstrap.bundle.min.js", "application/javascript");
}

void WebInterface::handleStyleCSS(AsyncWebServerRequest* request) {
    sendFile(request, "/style.css", "text/css");
}

void WebInterface::handleAppJS(AsyncWebServerRequest* request) {
    sendFile(request, "/app.js", "application/javascript");
}

void WebInterface::handleConfigGPIOJS(AsyncWebServerRequest* request) {
    sendFile(request, "/config-gpio.js", "application/javascript");
}

void WebInterface::handleConfigTimingJS(AsyncWebServerRequest* request) {
    sendFile(request, "/config-timing.js", "application/javascript");
}

void WebInterface::handleConfigSystemJS(AsyncWebServerRequest* request) {
    sendFile(request, "/config-system.js", "application/javascript");
}

// ============================================================================
// Helper Methods
// ============================================================================

String WebInterface::buildStatusJSON() {
    DynamicJsonDocument doc(2048);
    
    // Status
    JsonObject status = doc.createNestedObject("status");
    status["state"] = stateManager->getStateDescription();
    status["stateDescription"] = stateManager->getStateDescription();
    status["progress"] = stateManager->getProgressPercent();
    status["elapsedTime"] = stateManager->getElapsedTime();
    status["estimatedRemaining"] = stateManager->getEstimatedRemaining();
    
    // Sensors
    JsonObject sensors = doc.createNestedObject("sensors");
    sensors["gerkonCount"] = waterControl->getGerkonCount();
    sensors["gerkonState"] = false; // Would need actual pin reading
    
    // Actuators
    ActuatorManager::ActuatorStatus actStatus = actuatorManager->getStatus();
    JsonObject actuators = doc.createNestedObject("actuators");
    actuators["washengine"] = actStatus.washengine;
    actuators["pompa"] = actStatus.pompa;
    actuators["waterValve"] = actStatus.water_valve;
    actuators["powder"] = actStatus.powder;
    actuators["led"] = actStatus.led;
    
    // System info
    JsonObject system = doc.createNestedObject("system");
    system["uptime"] = millis();
    system["freeHeap"] = ESP.getFreeHeap();
    system["wifiRSSI"] = WiFi.RSSI();
    system["ipAddress"] = WiFi.localIP().toString();
    
    String output;
    serializeJson(doc, output);
    return output;
}

void WebInterface::sendFile(AsyncWebServerRequest* request, const char* path, const char* contentType) {
    if (LittleFS.exists(path)) {
        request->send(LittleFS, path, contentType);
    } else {
        Serial.printf("ERROR: File not found: %s\n", path);
        request->send(404, "text/plain", "File not found");
    }
}
