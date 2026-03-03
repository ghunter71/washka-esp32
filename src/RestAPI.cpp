/**
 * RestAPI.cpp
 * 
 * Implementation of REST API for wash system control.
 */

#include "RestAPI.h"
#include "PinLegend.h"

RestAPI::RestAPI() : 
    webServer(nullptr),
    configManager(nullptr),
    stateManager(nullptr),
    actuatorManager(nullptr),
    waterControl(nullptr),
    apiToken(""),
    authEnabled(false) {
}

RestAPI::~RestAPI() {
}

bool RestAPI::begin(AsyncWebServer* server, 
                    ConfigManager* config,
                    StateManager* state,
                    ActuatorManager* actuators,
                    WaterControl* water) {
    if (!server || !config || !state || !actuators || !water) {
        Serial.println("RestAPI: Invalid parameters");
        return false;
    }
    
    webServer = server;
    configManager = config;
    stateManager = state;
    actuatorManager = actuators;
    waterControl = water;
    
    // Register API endpoints
    
    // GET /api/status - Current system status
    webServer->on("/api/status", HTTP_GET, [this](AsyncWebServerRequest* request) {
        this->handleGetStatus(request);
    });
    
    // GET /api/config - Current configuration
    webServer->on("/api/config", HTTP_GET, [this](AsyncWebServerRequest* request) {
        this->handleGetConfig(request);
    });
    
    // POST /api/config/wifi - Update WiFi configuration (MUST be before /api/config)
    webServer->on("/api/config/wifi", HTTP_POST,
        [this](AsyncWebServerRequest* request) {
            // Response sent in body handler
        },
        nullptr,
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            if (index == 0) {
                this->handlePostWiFiConfig(request, data, len);
            }
        }
    );
    
    // POST /api/config/telegram - Update Telegram configuration (MUST be before /api/config)
    webServer->on("/api/config/telegram", HTTP_POST,
        [this](AsyncWebServerRequest* request) {
            // Response sent in body handler
        },
        nullptr,
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            if (index == 0) {
                this->handlePostTelegramConfig(request, data, len);
            }
        }
    );
    
    // POST /api/config/timing - Update timing configuration (MUST be before /api/config)
    webServer->on("/api/config/timing", HTTP_POST,
        [this](AsyncWebServerRequest* request) {
            // Response sent in body handler
        },
        nullptr,
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            if (index == 0) {
                this->handlePostTimingConfig(request, data, len);
            }
        }
    );
    
    // POST /api/config - Update configuration
    webServer->on("/api/config", HTTP_POST, 
        [this](AsyncWebServerRequest* request) {
            // Response sent in body handler
        },
        nullptr,
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            if (index == 0) {
                // First chunk - process the data
                this->handlePostConfig(request, data, len);
            }
        }
    );
    
    // POST /api/control/start - Start wash cycle
    webServer->on("/api/control/start", HTTP_POST, [this](AsyncWebServerRequest* request) {
        this->handleControlStart(request);
    });
    
    // POST /api/control/stop - Stop wash cycle
    webServer->on("/api/control/stop", HTTP_POST, [this](AsyncWebServerRequest* request) {
        this->handleControlStop(request);
    });
    
    // POST /api/control/pause - Pause wash cycle
    webServer->on("/api/control/pause", HTTP_POST, [this](AsyncWebServerRequest* request) {
        this->handleControlPause(request);
    });
    
    // POST /api/control/resume - Resume wash cycle
    webServer->on("/api/control/resume", HTTP_POST, [this](AsyncWebServerRequest* request) {
        this->handleControlResume(request);
    });
    
    // POST /api/control/manual - Manual actuator control
    webServer->on("/api/control/manual", HTTP_POST,
        [this](AsyncWebServerRequest* request) {
            // Response sent in body handler
        },
        nullptr,
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            if (index == 0) {
                this->handleControlManual(request, data, len);
            }
        }
    );
    
    // GET /api/docs - OpenAPI documentation
    webServer->on("/api/docs", HTTP_GET, [this](AsyncWebServerRequest* request) {
        this->handleDocs(request);
    });
    
    // GET /api/pins - Pin legend data
    webServer->on("/api/pins", HTTP_GET, [this](AsyncWebServerRequest* request) {
        this->handleGetPins(request);
    });
    
    // POST /api/system/restart - Restart device
    webServer->on("/api/system/restart", HTTP_POST, [this](AsyncWebServerRequest* request) {
        this->handleSystemRestart(request);
    });
    
    // GET /api/config/export - Export configuration
    webServer->on("/api/config/export", HTTP_GET, [this](AsyncWebServerRequest* request) {
        this->handleConfigExport(request);
    });
    
    // POST /api/config/import - Import configuration
    webServer->on("/api/config/import", HTTP_POST,
        [this](AsyncWebServerRequest* request) {
            // Response sent in body handler
        },
        nullptr,
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            if (index == 0) {
                this->handleConfigImport(request, data, len);
            }
        }
    );
    
    // POST /api/config/factory-reset - Factory reset
    webServer->on("/api/config/factory-reset", HTTP_POST, [this](AsyncWebServerRequest* request) {
        this->handleFactoryReset(request);
    });
    
    Serial.println("RestAPI: Initialized successfully");
    return true;
}

void RestAPI::setAPIToken(const String& token) {
    apiToken = token;
    authEnabled = (token.length() > 0);
    Serial.printf("RestAPI: Authentication %s\n", authEnabled ? "enabled" : "disabled");
}

String RestAPI::getAPIToken() {
    return apiToken;
}

bool RestAPI::isAuthenticationEnabled() {
    return authEnabled;
}

bool RestAPI::authenticateRequest(AsyncWebServerRequest* request) {
    if (!authEnabled) {
        return true; // No authentication required
    }
    
    // Check for Authorization header
    if (request->hasHeader("Authorization")) {
        String authHeader = request->header("Authorization");
        
        // Support "Bearer <token>" format
        if (authHeader.startsWith("Bearer ")) {
            String token = authHeader.substring(7);
            return (token == apiToken);
        }
        
        // Support direct token
        return (authHeader == apiToken);
    }
    
    // Check for api_token query parameter
    if (request->hasParam("api_token")) {
        String token = request->getParam("api_token")->value();
        return (token == apiToken);
    }
    
    return false;
}

void RestAPI::sendJsonResponse(AsyncWebServerRequest* request, int code, const String& json) {
    request->send(code, "application/json", json);
}

void RestAPI::sendErrorResponse(AsyncWebServerRequest* request, int code, const String& error) {
    StaticJsonDocument<200> doc;
    doc["error"] = error;
    doc["code"] = code;
    
    String response;
    serializeJson(doc, response);
    sendJsonResponse(request, code, response);
}

void RestAPI::handleGetStatus(AsyncWebServerRequest* request) {
    // Authenticate
    if (!authenticateRequest(request)) {
        sendErrorResponse(request, 401, "Unauthorized - Invalid or missing API token");
        return;
    }
    
    String json = buildStatusJson();
    sendJsonResponse(request, 200, json);
}

void RestAPI::handleGetConfig(AsyncWebServerRequest* request) {
    // Authenticate
    if (!authenticateRequest(request)) {
        sendErrorResponse(request, 401, "Unauthorized - Invalid or missing API token");
        return;
    }
    
    String json = buildConfigJson();
    sendJsonResponse(request, 200, json);
}

void RestAPI::handlePostConfig(AsyncWebServerRequest* request, uint8_t* data, size_t len) {
    // Authenticate
    if (!authenticateRequest(request)) {
        sendErrorResponse(request, 401, "Unauthorized - Invalid or missing API token");
        return;
    }
    
    // Parse JSON
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, data, len);
    
    if (error) {
        sendErrorResponse(request, 400, "Invalid JSON: " + String(error.c_str()));
        return;
    }
    
    bool configUpdated = false;
    String errorMsg = "";
    
    // Update GPIO pins if provided
    if (doc.containsKey("pins")) {
        ConfigManager::PinConfig pins;
        pins.washengine = doc["pins"]["washengine"] | pins.washengine;
        pins.pompa = doc["pins"]["pompa"] | pins.pompa;
        pins.watergerkon = doc["pins"]["watergerkon"] | pins.watergerkon;
        pins.powder = doc["pins"]["powder"] | pins.powder;
        pins.water_valve = doc["pins"]["water_valve"] | pins.water_valve;
        pins.button = doc["pins"]["button"] | pins.button;
        pins.led = doc["pins"]["led"] | pins.led;
        
        if (!configManager->setPinConfig(pins)) {
            errorMsg += "Invalid pin configuration. ";
        } else {
            configUpdated = true;
        }
    }
    
    // Update timing if provided
    if (doc.containsKey("timing")) {
        ConfigManager::TimingConfig timing;
        timing.tpomp = doc["timing"]["tpomp"] | timing.tpomp;
        timing.washtime0 = doc["timing"]["washtime0"] | timing.washtime0;
        timing.washtime1 = doc["timing"]["washtime1"] | timing.washtime1;
        timing.washtime2 = doc["timing"]["washtime2"] | timing.washtime2;
        timing.washtime3 = doc["timing"]["washtime3"] | timing.washtime3;
        timing.pausa = doc["timing"]["pausa"] | timing.pausa;
        timing.water_in_timer = doc["timing"]["water_in_timer"] | timing.water_in_timer;
        
        if (!configManager->setTimingConfig(timing)) {
            errorMsg += "Invalid timing configuration. ";
        } else {
            configUpdated = true;
        }
    }
    
    // Update gerkon settings if provided
    if (doc.containsKey("gerkon")) {
        if (doc["gerkon"].containsKey("threshold")) {
            uint16_t threshold = doc["gerkon"]["threshold"];
            if (threshold >= 1 && threshold <= 1000) {
                configManager->setGerkonThreshold(threshold);
                configUpdated = true;
            } else {
                errorMsg += "Invalid gerkon threshold (must be 1-1000). ";
            }
        }
        
        if (doc["gerkon"].containsKey("debounce_ms")) {
            uint16_t debounce = doc["gerkon"]["debounce_ms"];
            if (debounce >= 1 && debounce <= 1000) {
                configManager->setGerkonDebounceMs(debounce);
                configUpdated = true;
            } else {
                errorMsg += "Invalid debounce time (must be 1-1000ms). ";
            }
        }
    }
    
    if (errorMsg.length() > 0) {
        sendErrorResponse(request, 400, errorMsg);
    } else if (configUpdated) {
        StaticJsonDocument<100> response;
        response["success"] = true;
        response["message"] = "Configuration updated successfully";
        
        String json;
        serializeJson(response, json);
        sendJsonResponse(request, 200, json);
    } else {
        sendErrorResponse(request, 400, "No valid configuration provided");
    }
}

void RestAPI::handlePostWiFiConfig(AsyncWebServerRequest* request, uint8_t* data, size_t len) {
    // Parse JSON
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, data, len);
    
    if (error) {
        sendErrorResponse(request, 400, "Invalid JSON: " + String(error.c_str()));
        return;
    }
    
    // Validate SSID
    if (!doc.containsKey("ssid")) {
        sendErrorResponse(request, 400, "Missing SSID");
        return;
    }
    
    String ssid = doc["ssid"].as<String>();
    String password = doc.containsKey("password") ? doc["password"].as<String>() : "";
    
    // Validate SSID length
    if (ssid.length() == 0 || ssid.length() > 32) {
        sendErrorResponse(request, 400, "Invalid SSID length (must be 1-32 characters)");
        return;
    }
    
    // Validate password length (if provided)
    // Empty password is allowed for open networks
    if (password.length() > 0 && password.length() < 8) {
        sendErrorResponse(request, 400, "Invalid password length (must be empty or at least 8 characters for WPA)");
        return;
    }
    
    // Save credentials
    configManager->setWiFiCredentials(ssid, password);
    
    // Send success response
    StaticJsonDocument<200> response;
    response["success"] = true;
    response["message"] = "WiFi configuration saved successfully";
    
    String json;
    serializeJson(response, json);
    sendJsonResponse(request, 200, json);
}

void RestAPI::handleControlStart(AsyncWebServerRequest* request) {
    // Authenticate
    if (!authenticateRequest(request)) {
        sendErrorResponse(request, 401, "Unauthorized - Invalid or missing API token");
        return;
    }
    
    // Check if system is idle
    if (!stateManager->isIdle()) {
        sendErrorResponse(request, 409, "Cannot start - system is not idle");
        return;
    }
    
    // Start cycle
    bool started = stateManager->startCycle();
    
    if (started) {
        StaticJsonDocument<200> response;
        response["success"] = true;
        response["message"] = "Wash cycle started";
        response["state"] = stateManager->getStateDescription();
        
        String json;
        serializeJson(response, json);
        sendJsonResponse(request, 200, json);
    } else {
        sendErrorResponse(request, 500, "Failed to start wash cycle");
    }
}

void RestAPI::handleControlStop(AsyncWebServerRequest* request) {
    // Authenticate
    if (!authenticateRequest(request)) {
        sendErrorResponse(request, 401, "Unauthorized - Invalid or missing API token");
        return;
    }
    
    // Stop cycle
    bool stopped = stateManager->stopCycle();
    
    if (stopped) {
        StaticJsonDocument<200> response;
        response["success"] = true;
        response["message"] = "Wash cycle stopped";
        response["state"] = stateManager->getStateDescription();
        
        String json;
        serializeJson(response, json);
        sendJsonResponse(request, 200, json);
    } else {
        sendErrorResponse(request, 500, "Failed to stop wash cycle");
    }
}

void RestAPI::handleControlPause(AsyncWebServerRequest* request) {
    // Authenticate
    if (!authenticateRequest(request)) {
        sendErrorResponse(request, 401, "Unauthorized - Invalid or missing API token");
        return;
    }
    
    // Check if system is running
    if (!stateManager->isRunning()) {
        sendErrorResponse(request, 409, "Cannot pause - system is not running");
        return;
    }
    
    // Pause cycle
    bool paused = stateManager->pauseCycle();
    
    if (paused) {
        StaticJsonDocument<200> response;
        response["success"] = true;
        response["message"] = "Wash cycle paused";
        response["state"] = stateManager->getStateDescription();
        
        String json;
        serializeJson(response, json);
        sendJsonResponse(request, 200, json);
    } else {
        sendErrorResponse(request, 500, "Failed to pause wash cycle");
    }
}

void RestAPI::handleControlResume(AsyncWebServerRequest* request) {
    // Authenticate
    if (!authenticateRequest(request)) {
        sendErrorResponse(request, 401, "Unauthorized - Invalid or missing API token");
        return;
    }
    
    // Check if system is paused
    if (!stateManager->isPaused()) {
        sendErrorResponse(request, 409, "Cannot resume - system is not paused");
        return;
    }
    
    // Resume cycle
    bool resumed = stateManager->resumeCycle();
    
    if (resumed) {
        StaticJsonDocument<200> response;
        response["success"] = true;
        response["message"] = "Wash cycle resumed";
        response["state"] = stateManager->getStateDescription();
        
        String json;
        serializeJson(response, json);
        sendJsonResponse(request, 200, json);
    } else {
        sendErrorResponse(request, 500, "Failed to resume wash cycle");
    }
}

void RestAPI::handleDocs(AsyncWebServerRequest* request) {
    String docs = buildOpenAPIDoc();
    request->send(200, "application/json", docs);
}

void RestAPI::handleGetPins(AsyncWebServerRequest* request) {
    // Return pin legend data
    String json = PinLegend::getPinLegendJson();
    sendJsonResponse(request, 200, json);
}

String RestAPI::buildStatusJson() {
    DynamicJsonDocument doc(1024);
    
    // Status section
    JsonObject status = doc.createNestedObject("status");
    status["state"] = static_cast<uint8_t>(stateManager->getCurrentState());
    status["stateDescription"] = stateManager->getStateDescription();
    status["progress"] = stateManager->getProgressPercent();
    status["elapsedTime"] = stateManager->getElapsedTime();
    status["estimatedRemaining"] = stateManager->getEstimatedRemaining();
    status["isIdle"] = stateManager->isIdle();
    status["isRunning"] = stateManager->isRunning();
    status["isPaused"] = stateManager->isPaused();
    status["isComplete"] = stateManager->isComplete();
    status["isError"] = stateManager->isError();
    
    // Sensors section
    JsonObject sensors = doc.createNestedObject("sensors");
    sensors["gerkonCount"] = waterControl->getGerkonCount();
    
    // Actuators section
    ActuatorManager::ActuatorStatus actuatorStatus = actuatorManager->getStatus();
    JsonObject actuators = doc.createNestedObject("actuators");
    actuators["washengine"] = actuatorStatus.washengine;
    actuators["pompa"] = actuatorStatus.pompa;
    actuators["waterValve"] = actuatorStatus.water_valve;
    actuators["powder"] = actuatorStatus.powder;
    actuators["led"] = actuatorStatus.led;
    
    // System section
    JsonObject system = doc.createNestedObject("system");
    system["uptime"] = millis();
    system["freeHeap"] = ESP.getFreeHeap();
    system["wifiRSSI"] = WiFi.RSSI();
    system["ipAddress"] = WiFi.localIP().toString();
    
    String json;
    serializeJson(doc, json);
    return json;
}

String RestAPI::buildConfigJson() {
    DynamicJsonDocument doc(1024);
    
    // GPIO pins
    ConfigManager::PinConfig pins = configManager->getPinConfig();
    JsonObject pinsObj = doc.createNestedObject("pins");
    pinsObj["washengine"] = pins.washengine;
    pinsObj["pompa"] = pins.pompa;
    pinsObj["watergerkon"] = pins.watergerkon;
    pinsObj["powder"] = pins.powder;
    pinsObj["water_valve"] = pins.water_valve;
    pinsObj["button"] = pins.button;
    pinsObj["led"] = pins.led;
    
    // Timing configuration
    ConfigManager::TimingConfig timing = configManager->getTimingConfig();
    JsonObject timingObj = doc.createNestedObject("timing");
    timingObj["tpomp"] = timing.tpomp;
    timingObj["washtime0"] = timing.washtime0;
    timingObj["washtime1"] = timing.washtime1;
    timingObj["washtime2"] = timing.washtime2;
    timingObj["washtime3"] = timing.washtime3;
    timingObj["pausa"] = timing.pausa;
    timingObj["water_in_timer"] = timing.water_in_timer;
    
    // Gerkon configuration
    JsonObject gerkon = doc.createNestedObject("gerkon");
    gerkon["threshold"] = configManager->getGerkonThreshold();
    gerkon["debounce_ms"] = configManager->getGerkonDebounceMs();
    
    // Telegram configuration
    JsonObject telegram = doc.createNestedObject("telegram");
    telegram["enabled"] = configManager->isTelegramEnabled();
    telegram["token"] = configManager->getTelegramToken();
    JsonArray chatIds = telegram.createNestedArray("chatIds");
    std::vector<int64_t> allowedIds = configManager->getAllowedChatIds();
    for (int64_t id : allowedIds) {
        chatIds.add(id);
    }
    
    String json;
    serializeJson(doc, json);
    return json;
}

String RestAPI::buildOpenAPIDoc() {
    // Simplified OpenAPI 3.0 documentation
    DynamicJsonDocument doc(4096);
    
    doc["openapi"] = "3.0.0";
    
    JsonObject info = doc.createNestedObject("info");
    info["title"] = "Washka System API";
    info["version"] = "1.0.0";
    info["description"] = "REST API for ESP32 dishwasher control system";
    
    JsonObject paths = doc.createNestedObject("paths");
    
    // GET /api/status
    JsonObject statusPath = paths.createNestedObject("/api/status");
    JsonObject statusGet = statusPath.createNestedObject("get");
    statusGet["summary"] = "Get current system status";
    statusGet["description"] = "Returns current state, progress, sensor readings, and actuator status";
    JsonArray statusTags = statusGet.createNestedArray("tags");
    statusTags.add("Status");
    
    // GET /api/config
    JsonObject configPath = paths.createNestedObject("/api/config");
    JsonObject configGet = configPath.createNestedObject("get");
    configGet["summary"] = "Get current configuration";
    configGet["description"] = "Returns GPIO pins, timing parameters, and gerkon settings";
    JsonArray configTags = configGet.createNestedArray("tags");
    configTags.add("Configuration");
    
    // POST /api/config
    JsonObject configPost = configPath.createNestedObject("post");
    configPost["summary"] = "Update configuration";
    configPost["description"] = "Update GPIO pins, timing parameters, or gerkon settings";
    JsonArray configPostTags = configPost.createNestedArray("tags");
    configPostTags.add("Configuration");
    
    // POST /api/control/start
    JsonObject startPath = paths.createNestedObject("/api/control/start");
    JsonObject startPost = startPath.createNestedObject("post");
    startPost["summary"] = "Start wash cycle";
    startPost["description"] = "Starts the wash cycle from IDLE state";
    JsonArray startTags = startPost.createNestedArray("tags");
    startTags.add("Control");
    
    // POST /api/control/stop
    JsonObject stopPath = paths.createNestedObject("/api/control/stop");
    JsonObject stopPost = stopPath.createNestedObject("post");
    stopPost["summary"] = "Stop wash cycle";
    stopPost["description"] = "Stops the wash cycle and returns to IDLE state";
    JsonArray stopTags = stopPost.createNestedArray("tags");
    stopTags.add("Control");
    
    // POST /api/control/pause
    JsonObject pausePath = paths.createNestedObject("/api/control/pause");
    JsonObject pausePost = pausePath.createNestedObject("post");
    pausePost["summary"] = "Pause wash cycle";
    pausePost["description"] = "Pauses the currently running wash cycle";
    JsonArray pauseTags = pausePost.createNestedArray("tags");
    pauseTags.add("Control");
    
    // Security schemes
    JsonObject components = doc.createNestedObject("components");
    JsonObject securitySchemes = components.createNestedObject("securitySchemes");
    JsonObject bearerAuth = securitySchemes.createNestedObject("bearerAuth");
    bearerAuth["type"] = "http";
    bearerAuth["scheme"] = "bearer";
    bearerAuth["description"] = "API token authentication";
    
    String json;
    serializeJson(doc, json);
    return json;
}

void RestAPI::handlePostTelegramConfig(AsyncWebServerRequest* request, uint8_t* data, size_t len) {
    // Parse JSON
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, data, len);
    
    if (error) {
        sendErrorResponse(request, 400, "Invalid JSON: " + String(error.c_str()));
        return;
    }
    
    bool configUpdated = false;
    
    // Update Telegram enabled flag if provided
    if (doc.containsKey("enabled")) {
        bool enabled = doc["enabled"].as<bool>();
        configManager->setTelegramEnabled(enabled);
        configUpdated = true;
    }
    
    // Update Telegram token if provided
    if (doc.containsKey("token")) {
        String token = doc["token"].as<String>();
        if (token.length() > 0) {
            configManager->setTelegramToken(token);
            configUpdated = true;
        }
    }
    
    // Update chat IDs if provided
    if (doc.containsKey("chatIds") && doc["chatIds"].is<JsonArray>()) {
        // Clear existing chat IDs
        configManager->clearAllowedChatIds();
        
        // Add new chat IDs
        JsonArray chatIds = doc["chatIds"].as<JsonArray>();
        for (JsonVariant chatId : chatIds) {
            if (chatId.is<int64_t>()) {
                configManager->addAllowedChatId(chatId.as<int64_t>());
                configUpdated = true;
            } else if (chatId.is<String>()) {
                // Try to parse string as int64
                String chatIdStr = chatId.as<String>();
                int64_t chatIdInt = chatIdStr.toInt();
                if (chatIdInt != 0 || chatIdStr == "0") {
                    configManager->addAllowedChatId(chatIdInt);
                    configUpdated = true;
                }
            }
        }
    }
    
    if (configUpdated) {
        StaticJsonDocument<200> response;
        response["success"] = true;
        response["message"] = "Telegram configuration saved successfully";
        
        String json;
        serializeJson(response, json);
        sendJsonResponse(request, 200, json);
    } else {
        sendErrorResponse(request, 400, "No valid Telegram configuration provided");
    }
}

void RestAPI::handleSystemRestart(AsyncWebServerRequest* request) {
    // Send success response first
    StaticJsonDocument<200> response;
    response["success"] = true;
    response["message"] = "Device restarting...";
    
    String json;
    serializeJson(response, json);
    sendJsonResponse(request, 200, json);
    
    // Delay to allow response to be sent
    delay(1000);
    
    // Restart ESP32
    Serial.println("System restart requested via API");
    ESP.restart();
}

void RestAPI::handlePostTimingConfig(AsyncWebServerRequest* request, uint8_t* data, size_t len) {
    // Parse JSON
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, data, len);
    
    if (error) {
        sendErrorResponse(request, 400, "Invalid JSON: " + String(error.c_str()));
        return;
    }
    
    // Extract timing configuration - check if data is nested under "timing" key
    ConfigManager::TimingConfig timing = configManager->getTimingConfig();
    
    JsonObject timingObj = doc.containsKey("timing") ? doc["timing"].as<JsonObject>() : doc.as<JsonObject>();
    
    if (timingObj.containsKey("tpomp")) timing.tpomp = timingObj["tpomp"];
    if (timingObj.containsKey("washtime0")) timing.washtime0 = timingObj["washtime0"];
    if (timingObj.containsKey("washtime1")) timing.washtime1 = timingObj["washtime1"];
    if (timingObj.containsKey("washtime2")) timing.washtime2 = timingObj["washtime2"];
    if (timingObj.containsKey("washtime3")) timing.washtime3 = timingObj["washtime3"];
    if (timingObj.containsKey("pausa")) timing.pausa = timingObj["pausa"];
    if (timingObj.containsKey("water_in_timer")) timing.water_in_timer = timingObj["water_in_timer"];
    
    // Also handle gerkon settings if present
    if (doc.containsKey("gerkon")) {
        JsonObject gerkonObj = doc["gerkon"].as<JsonObject>();
        if (gerkonObj.containsKey("threshold")) {
            configManager->setGerkonThreshold(gerkonObj["threshold"]);
        }
        if (gerkonObj.containsKey("debounce_ms")) {
            configManager->setGerkonDebounceMs(gerkonObj["debounce_ms"]);
        }
    }
    
    // Validate and save
    if (configManager->setTimingConfig(timing)) {
        StaticJsonDocument<200> response;
        response["success"] = true;
        response["message"] = "Timing configuration saved successfully";
        
        String json;
        serializeJson(response, json);
        sendJsonResponse(request, 200, json);
    } else {
        sendErrorResponse(request, 400, "Invalid timing configuration");
    }
}

void RestAPI::handleControlManual(AsyncWebServerRequest* request, uint8_t* data, size_t len) {
    // Parse JSON
    DynamicJsonDocument doc(256);
    DeserializationError error = deserializeJson(doc, data, len);
    
    if (error) {
        sendErrorResponse(request, 400, "Invalid JSON: " + String(error.c_str()));
        return;
    }
    
    if (!doc.containsKey("actuator") || !doc.containsKey("state")) {
        sendErrorResponse(request, 400, "Missing actuator or state");
        return;
    }
    
    String actuator = doc["actuator"].as<String>();
    bool state = doc["state"].as<bool>();
    
    // Control actuator
    bool success = false;
    if (actuator == "washengine") {
        actuatorManager->setWashEngine(state);
        success = true;
    } else if (actuator == "pompa" || actuator == "pump") {
        actuatorManager->setPump(state);
        success = true;
    } else if (actuator == "waterValve" || actuator == "valve") {
        actuatorManager->setWaterValve(state);
        success = true;
    } else if (actuator == "powder") {
        actuatorManager->setPowderDispenser(state);
        success = true;
    } else if (actuator == "led") {
        actuatorManager->setLED(state);
        success = true;
    }
    
    if (success) {
        StaticJsonDocument<200> response;
        response["success"] = true;
        response["message"] = "Actuator controlled successfully";
        response["actuator"] = actuator;
        response["state"] = state;
        
        String json;
        serializeJson(response, json);
        sendJsonResponse(request, 200, json);
    } else {
        sendErrorResponse(request, 400, "Unknown actuator: " + actuator);
    }
}

void RestAPI::handleConfigExport(AsyncWebServerRequest* request) {
    String json = configManager->exportToJson();
    sendJsonResponse(request, 200, json);
}

void RestAPI::handleConfigImport(AsyncWebServerRequest* request, uint8_t* data, size_t len) {
    // Parse JSON
    String jsonStr;
    jsonStr.reserve(len);
    for (size_t i = 0; i < len; i++) {
        jsonStr += (char)data[i];
    }
    
    if (configManager->importFromJson(jsonStr)) {
        StaticJsonDocument<200> response;
        response["success"] = true;
        response["message"] = "Configuration imported successfully";
        
        String json;
        serializeJson(response, json);
        sendJsonResponse(request, 200, json);
    } else {
        sendErrorResponse(request, 400, "Failed to import configuration");
    }
}

void RestAPI::handleFactoryReset(AsyncWebServerRequest* request) {
    // Perform factory reset
    configManager->factoryReset();
    
    // Send success response
    StaticJsonDocument<200> response;
    response["success"] = true;
    response["message"] = "Factory reset completed. Device will restart...";
    
    String json;
    serializeJson(response, json);
    sendJsonResponse(request, 200, json);
    
    // Delay to allow response to be sent
    delay(1000);
    
    // Restart ESP32
    Serial.println("Factory reset completed, restarting...");
    ESP.restart();
}
