/**
 * WiFiManager.cpp
 * 
 * Implementation of WiFi management with AP mode and captive portal.
 */

#include "WiFiManager.h"
#include <DNSServer.h>
#include <time.h>

// Static constants
const char* WiFiManager::AP_PASSWORD = nullptr;  // Open network (no password)

// DNS server for captive portal
static DNSServer dnsServer;
static bool dnsStarted = false;

WiFiManager::WiFiManager() :
    configManager(nullptr),
    webServer(nullptr),
    status(ConnectionStatus::FAILED),
    apSSID("Washka-Setup"),
    apMode(false),
    lastConnectionAttempt(0),
    connectionAttempts(0) {
}

WiFiManager::~WiFiManager() {
    if (apMode) {
        stopConfigPortal();
    }
    WiFi.disconnect(true);
}

bool WiFiManager::begin(ConfigManager* config, AsyncWebServer* server) {
    configManager = config;
    webServer = server;
    
    // Set WiFi mode
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    
    Serial.println("✓ WiFiManager initialized");
    
    // Try to connect if credentials are available
    if (configManager->hasWiFiCredentials()) {
        return connect();
    } else {
        Serial.println("No WiFi credentials found, starting AP mode");
        startConfigPortal();
        return true;
    }
}

// ============================================================================
// Connection Management
// ============================================================================

bool WiFiManager::connect() {
    if (!configManager->hasWiFiCredentials()) {
        Serial.println("ERROR: No WiFi credentials configured");
        startConfigPortal();
        return false;
    }
    
    String ssid = configManager->getWiFiSSID();
    String password = configManager->getWiFiPassword();
    
    Serial.printf("Connecting to WiFi: %s\n", ssid.c_str());
    
    status = ConnectionStatus::CONNECTING;
    connectionAttempts = 0;
    
    WiFi.begin(ssid.c_str(), password.c_str());
    
    // Wait for connection
    while (WiFi.status() != WL_CONNECTED && connectionAttempts < MAX_CONNECTION_ATTEMPTS) {
        delay(CONNECTION_TIMEOUT);
        connectionAttempts++;
        Serial.print(".");
    }
    Serial.println();
    
    if (WiFi.status() == WL_CONNECTED) {
        status = ConnectionStatus::CONNECTED;
        Serial.printf("✓ Connected to WiFi: %s\n", ssid.c_str());
        Serial.printf("  IP address: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("  RSSI: %d dBm\n", WiFi.RSSI());
        
        // Configure NTP time synchronization for SSL/TLS
        configTime(0, 0, "pool.ntp.org", "time.nist.gov");
        Serial.println("✓ NTP time sync configured");
        
        return true;
    } else {
        status = ConnectionStatus::FAILED;
        Serial.println("ERROR: Failed to connect to WiFi");
        Serial.println("Starting AP mode for configuration");
        startConfigPortal();
        return false;
    }
}

void WiFiManager::disconnect() {
    WiFi.disconnect(true);
    status = ConnectionStatus::FAILED;
    Serial.println("WiFi disconnected");
}

// ============================================================================
// Status
// ============================================================================

WiFiManager::ConnectionStatus WiFiManager::getStatus() {
    // Update status based on WiFi state
    if (apMode) {
        return ConnectionStatus::AP_MODE;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        status = ConnectionStatus::CONNECTED;
    } else if (WiFi.status() == WL_DISCONNECTED) {
        status = ConnectionStatus::FAILED;
    }
    
    return status;
}

String WiFiManager::getIPAddress() {
    if (apMode) {
        return WiFi.softAPIP().toString();
    }
    return WiFi.localIP().toString();
}

String WiFiManager::getAPSSID() {
    return apSSID;
}

int WiFiManager::getRSSI() {
    if (WiFi.status() == WL_CONNECTED) {
        return WiFi.RSSI();
    }
    return 0;
}

// ============================================================================
// Access Point Mode
// ============================================================================

void WiFiManager::startConfigPortal() {
    if (apMode) {
        Serial.println("AP mode already active");
        return;
    }
    
    Serial.println("Starting Access Point mode");
    
    // Stop any existing WiFi connection
    WiFi.disconnect(true);
    delay(100);
    
    // Start AP mode (open network - no password)
    WiFi.mode(WIFI_AP);
    bool apStarted = WiFi.softAP(apSSID.c_str());
    
    if (!apStarted) {
        Serial.println("ERROR: Failed to start AP mode");
        return;
    }
    
    apMode = true;
    status = ConnectionStatus::AP_MODE;
    
    IPAddress apIP = WiFi.softAPIP();
    Serial.printf("✓ AP mode started\n");
    Serial.printf("  SSID: %s (Open Network)\n", apSSID.c_str());
    Serial.printf("  IP address: %s\n", apIP.toString().c_str());
    
    // Setup captive portal
    setupCaptivePortal();
}

void WiFiManager::stopConfigPortal() {
    if (!apMode) {
        return;
    }
    
    Serial.println("Stopping Access Point mode");
    
    // Stop DNS server
    if (dnsStarted) {
        dnsServer.stop();
        dnsStarted = false;
    }
    
    // Stop AP
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
    
    apMode = false;
    
    Serial.println("✓ AP mode stopped");
}

bool WiFiManager::isAPMode() {
    return apMode;
}

// ============================================================================
// Update Loop
// ============================================================================

void WiFiManager::loop() {
    // Process DNS requests for captive portal
    if (apMode && dnsStarted) {
        dnsServer.processNextRequest();
    }
    
    // Check connection status
    if (!apMode && WiFi.status() != WL_CONNECTED) {
        unsigned long now = millis();
        if (now - lastConnectionAttempt > 30000) { // Retry every 30 seconds
            lastConnectionAttempt = now;
            Serial.println("WiFi connection lost, attempting to reconnect...");
            connect();
        }
    }
}

// ============================================================================
// Private Helper Methods
// ============================================================================

void WiFiManager::setupCaptivePortal() {
    if (!webServer) {
        Serial.println("ERROR: Web server not initialized");
        return;
    }
    
    // Start DNS server for captive portal
    dnsServer.start(53, "*", WiFi.softAPIP());
    dnsStarted = true;
    
    // Setup WiFi configuration page
    webServer->on("/wifi", HTTP_GET, [this](AsyncWebServerRequest* request) {
        String html = R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Washka WiFi Setup</title>
    <style>
        body { font-family: Arial, sans-serif; max-width: 400px; margin: 50px auto; padding: 20px; }
        h1 { color: #333; }
        input { width: 100%; padding: 10px; margin: 10px 0; box-sizing: border-box; }
        button { width: 100%; padding: 12px; background: #007bff; color: white; border: none; cursor: pointer; }
        button:hover { background: #0056b3; }
        .info { background: #f0f0f0; padding: 10px; margin: 10px 0; border-radius: 5px; }
    </style>
</head>
<body>
    <h1>Washka WiFi Setup</h1>
    <div class="info">
        <p>Configure WiFi credentials to connect your Washka device to your network.</p>
    </div>
    <form action="/wifi" method="POST">
        <input type="text" name="ssid" placeholder="WiFi SSID" required>
        <input type="password" name="password" placeholder="WiFi Password" required>
        <button type="submit">Connect</button>
    </form>
</body>
</html>
        )";
        request->send(200, "text/html", html);
    });
    
    // Handle WiFi configuration submission
    webServer->on("/wifi", HTTP_POST, [this](AsyncWebServerRequest* request) {
        // Response sent in body handler
    }, nullptr, [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
        if (index == 0) {
            handleWiFiConfig(request, data, len);
        }
    });
    
    // Redirect all other requests to WiFi config page (captive portal)
    webServer->onNotFound([](AsyncWebServerRequest* request) {
        request->redirect("/wifi");
    });
    
    Serial.println("✓ Captive portal configured");
}

void WiFiManager::handleWiFiConfig(AsyncWebServerRequest* request, uint8_t* data, size_t len) {
    // Parse JSON body
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, data, len);
    
    if (error) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
        return;
    }
    
    if (!doc.containsKey("ssid")) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Missing SSID\"}");
        return;
    }
    
    String ssid = doc["ssid"].as<String>();
    String password = doc.containsKey("password") ? doc["password"].as<String>() : "";
    
    Serial.printf("Received WiFi credentials: SSID=%s\n", ssid.c_str());
    
    // Validate SSID
    if (ssid.length() == 0 || ssid.length() > 32) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Invalid SSID length (1-32 characters)\"}");
        return;
    }
    
    // Save credentials
    configManager->setWiFiCredentials(ssid, password);
    
    // Send success response
    request->send(200, "application/json", "{\"success\":true,\"message\":\"WiFi configured successfully\"}");
    
    // Stop AP mode and connect to WiFi after a short delay
    delay(2000);
    stopConfigPortal();
    connect();
}
