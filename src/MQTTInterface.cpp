/**
 * MQTTInterface.cpp
 * 
 * Implementation of MQTT integration for Home Assistant.
 * 
 * @version 1.0.0
 */

#include "MQTTInterface.h"
#include "ConfigManager.h"
#include "StateManager.h"
#include "ActuatorManager.h"
#include "WaterControl.h"
#include <ESPAsyncWebServer.h>
#include <AsyncMqttClient.h>
#include <ArduinoJson.h>

MQTTInterface::MQTTInterface() 
    : mqttClient(nullptr),
      configManager(nullptr),
      stateManager(nullptr),
      actuatorManager(nullptr),
      waterControl(nullptr),
      status(ConnectionStatus::DISCONNECTED),
      startCallback(nullptr),
      stopCallback(nullptr),
      pauseCallback(nullptr),
      resumeCallback(nullptr),
      lastPublishTime(0),
      lastReconnectAttempt(0) {
}

MQTTInterface::~MQTTInterface() {
    if (mqttClient) {
        delete mqttClient;
    }
}

bool MQTTInterface::begin(ConfigManager* config, StateManager* state, 
                          ActuatorManager* actuator, WaterControl* water) {
    configManager = config;
    stateManager = state;
    actuatorManager = actuator;
    waterControl = water;
    
    // Create MQTT client
    mqttClient = new AsyncMqttClient();
    if (!mqttClient) {
        Serial.println("ERROR: Failed to create MQTT client");
        return false;
    }
    
    // Set callbacks
    mqttClient->onConnect([this](bool sessionPresent) { onConnect(sessionPresent); });
    mqttClient->onDisconnect([this](AsyncMqttClientDisconnectReason reason) { onDisconnect(reason); });
    mqttClient->onMessage([this](char* topic, char* payload, 
                                  AsyncMqttClientMessageProperties properties,
                                  size_t len, size_t index, size_t total) {
        onMessage(topic, payload, properties, len, index, total);
    });
    mqttClient->onPublish([this](uint16_t packetId) { onPublish(packetId); });
    
    // Load configuration
    loadConfig();
    
    Serial.println("✓ MQTTInterface initialized");
    return true;
}

// ============================================================================
// Configuration
// ============================================================================

void MQTTInterface::setConfig(const MQTTConfig& config) {
    mqttConfig = config;
}

MQTTInterface::MQTTConfig MQTTInterface::getConfig() {
    return mqttConfig;
}

bool MQTTInterface::saveConfig() {
    // Save to ConfigManager's NVS
    // This would be implemented with ConfigManager extension
    return true;
}

bool MQTTInterface::loadConfig() {
    // Load from ConfigManager's NVS
    // This would be implemented with ConfigManager extension
    return true;
}

// ============================================================================
// Connection Management
// ============================================================================

void MQTTInterface::connect() {
    if (!mqttClient || mqttConfig.server.isEmpty()) {
        return;
    }
    
    status = ConnectionStatus::CONNECTING;
    
    // Configure client
    mqttClient->setServer(mqttConfig.server.c_str(), mqttConfig.port);
    
    if (!mqttConfig.username.isEmpty()) {
        mqttClient->setCredentials(mqttConfig.username.c_str(), 
                                   mqttConfig.password.c_str());
    }
    
    mqttClient->setClientId(mqttConfig.clientId.c_str());
    mqttClient->setKeepAlive(mqttConfig.keepAlive);
    
    // Set will (LWT - Last Will Testament)
    mqttClient->setWill(getAvailabilityTopic().c_str(), 1, true, "offline");
    
    Serial.printf("MQTT: Connecting to %s:%d...\n", 
                  mqttConfig.server.c_str(), mqttConfig.port);
    
    mqttClient->connect();
}

void MQTTInterface::disconnect() {
    if (mqttClient && mqttClient->connected()) {
        publishAvailability(false);
        mqttClient->disconnect();
    }
    status = ConnectionStatus::DISCONNECTED;
}

MQTTInterface::ConnectionStatus MQTTInterface::getStatus() {
    return status;
}

bool MQTTInterface::isConnected() {
    return mqttClient && mqttClient->connected();
}

// ============================================================================
// Main Loop
// ============================================================================

void MQTTInterface::loop() {
    if (!mqttConfig.enabled) {
        return;
    }
    
    unsigned long now = millis();
    
    // Handle reconnection
    if (!isConnected() && now - lastReconnectAttempt >= RECONNECT_INTERVAL) {
        lastReconnectAttempt = now;
        connect();
    }
    
    // Periodic state publishing
    if (isConnected() && now - lastPublishTime >= PUBLISH_INTERVAL) {
        lastPublishTime = now;
        publishState();
    }
}

// ============================================================================
// State Publishing
// ============================================================================

void MQTTInterface::publishState() {
    if (!isConnected() || !stateManager) return;
    
    StaticJsonDocument<512> doc;
    
    doc["state"] = stateManager->getStateDescription();
    doc["progress"] = stateManager->getProgressPercent();
    doc["elapsed"] = stateManager->getElapsedTime() / 1000;  // seconds
    doc["remaining"] = stateManager->getEstimatedRemaining() / 1000;
    doc["paused"] = stateManager->isPaused();
    doc["running"] = stateManager->isRunning();
    
    String payload;
    serializeJson(doc, payload);
    
    mqttClient->publish(getStateTopic().c_str(), 1, true, payload.c_str());
}

void MQTTInterface::publishStatus() {
    if (!isConnected()) return;
    
    StaticJsonDocument<256> doc;
    
    doc["ip"] = WiFi.localIP().toString();
    doc["rssi"] = WiFi.RSSI();
    doc["uptime"] = millis() / 1000;
    doc["free_heap"] = ESP.getFreeHeap();
    
    String payload;
    serializeJson(doc, payload);
    
    mqttClient->publish(getTopic("status").c_str(), 1, false, payload.c_str());
}

void MQTTInterface::publishAvailability(bool online) {
    if (!isConnected()) return;
    
    mqttClient->publish(getAvailabilityTopic().c_str(), 1, true, 
                        online ? "online" : "offline");
}

void MQTTInterface::publishSensorData() {
    if (!isConnected() || !waterControl) return;
    
    StaticJsonDocument<256> doc;
    
    doc["gerkon_count"] = waterControl->getGerkonCount();
    doc["gerkon_debounce"] = waterControl->getDebounceMs();
    
    String payload;
    serializeJson(doc, payload);
    
    mqttClient->publish(getTopic("sensors").c_str(), 1, false, payload.c_str());
}

// ============================================================================
// Home Assistant Discovery
// ============================================================================

void MQTTInterface::publishDiscovery() {
    if (!isConnected() || !mqttConfig.discoveryEnabled) return;
    
    publishSwitchDiscovery();
    publishSensorDiscovery();
    publishBinarySensorDiscovery();
    
    Serial.println("✓ MQTT discovery published");
}

void MQTTInterface::clearDiscovery() {
    if (!isConnected()) return;
    
    // Publish empty config to remove devices
    String topics[] = {
        getConfigTopic("switch", "washka"),
        getConfigTopic("sensor", "washka_state"),
        getConfigTopic("sensor", "washka_progress"),
        getConfigTopic("binary_sensor", "washka_running"),
        getConfigTopic("binary_sensor", "washka_paused")
    };
    
    for (const auto& topic : topics) {
        mqttClient->publish(topic.c_str(), 1, true, "");
    }
}

void MQTTInterface::publishSwitchDiscovery() {
    // Main wash switch
    StaticJsonDocument<512> doc;
    
    doc["name"] = "Washka Dishwasher";
    doc["unique_id"] = "washka_switch";
    doc["~"] = mqttConfig.topicPrefix + "/washka";
    doc["cmd_t"] = "~/command";
    doc["stat_t"] = "~/state";
    doc["avty_t"] = "~/availability";
    doc["pl_on"] = "START";
    doc["pl_off"] = "STOP";
    doc["stat_on"] = "running";
    doc["stat_off"] = "idle";
    doc["device"]["identifiers"] = "washka-esp32";
    doc["device"]["name"] = "Washka Dishwasher";
    doc["device"]["manufacturer"] = "Washka";
    doc["device"]["model"] = "ESP32 Controller";
    doc["device"]["sw_version"] = "2.0.0";
    
    String payload;
    serializeJson(doc, payload);
    
    mqttClient->publish(getConfigTopic("switch", "washka").c_str(), 
                        1, true, payload.c_str());
}

void MQTTInterface::publishSensorDiscovery() {
    // State sensor
    StaticJsonDocument<512> doc;
    
    doc["name"] = "Washka State";
    doc["unique_id"] = "washka_state";
    doc["~"] = mqttConfig.topicPrefix + "/washka";
    doc["stat_t"] = "~/state";
    doc["avty_t"] = "~/availability";
    doc["value_template"] = "{{ value_json.state }}";
    doc["device"]["identifiers"] = "washka-esp32";
    
    String payload;
    serializeJson(doc, payload);
    
    mqttClient->publish(getConfigTopic("sensor", "washka_state").c_str(), 
                        1, true, payload.c_str());
    
    // Progress sensor
    doc["name"] = "Washka Progress";
    doc["unique_id"] = "washka_progress";
    doc["value_template"] = "{{ value_json.progress }}";
    doc["unit_of_measurement"] = "%";
    doc["device_class"] = "progress";
    
    serializeJson(doc, payload);
    mqttClient->publish(getConfigTopic("sensor", "washka_progress").c_str(), 
                        1, true, payload.c_str());
}

void MQTTInterface::publishBinarySensorDiscovery() {
    // Running sensor
    StaticJsonDocument<512> doc;
    
    doc["name"] = "Washka Running";
    doc["unique_id"] = "washka_running";
    doc["~"] = mqttConfig.topicPrefix + "/washka";
    doc["stat_t"] = "~/state";
    doc["avty_t"] = "~/availability";
    doc["value_template"] = "{{ value_json.running }}";
    doc["payload_on"] = "true";
    doc["payload_off"] = "false";
    doc["device_class"] = "running";
    doc["device"]["identifiers"] = "washka-esp32";
    
    String payload;
    serializeJson(doc, payload);
    
    mqttClient->publish(getConfigTopic("binary_sensor", "washka_running").c_str(), 
                        1, true, payload.c_str());
    
    // Paused sensor
    doc["name"] = "Washka Paused";
    doc["unique_id"] = "washka_paused";
    doc["value_template"] = "{{ value_json.paused }}";
    doc["device_class"] = "power";
    
    serializeJson(doc, payload);
    mqttClient->publish(getConfigTopic("binary_sensor", "washka_paused").c_str(), 
                        1, true, payload.c_str());
}

// ============================================================================
// Command Registration
// ============================================================================

void MQTTInterface::onStartCommand(CommandCallback callback) {
    startCallback = callback;
}

void MQTTInterface::onStopCommand(CommandCallback callback) {
    stopCallback = callback;
}

void MQTTInterface::onPauseCommand(CommandCallback callback) {
    pauseCallback = callback;
}

void MQTTInterface::onResumeCommand(CommandCallback callback) {
    resumeCallback = callback;
}

// ============================================================================
// Topic Helpers
// ============================================================================

String MQTTInterface::getTopic(const String& suffix) {
    return mqttConfig.topicPrefix + "/washka/" + suffix;
}

String MQTTInterface::getStateTopic() {
    return getTopic("state");
}

String MQTTInterface::getCommandTopic() {
    return getTopic("command");
}

String MQTTInterface::getAvailabilityTopic() {
    return getTopic("availability");
}

String MQTTInterface::getConfigTopic(const String& component, const String& objectId) {
    return mqttConfig.topicPrefix + "/" + component + "/washka/" + objectId + "/config";
}

// ============================================================================
// MQTT Callbacks
// ============================================================================

void MQTTInterface::onConnect(bool sessionPresent) {
    status = ConnectionStatus::CONNECTED;
    
    Serial.println("✓ MQTT Connected");
    
    // Subscribe to command topic
    mqttClient->subscribe(getCommandTopic().c_str(), 1);
    
    // Publish availability
    publishAvailability(true);
    
    // Publish discovery if enabled
    if (mqttConfig.discoveryEnabled) {
        publishDiscovery();
    }
    
    // Publish initial state
    publishState();
}

void MQTTInterface::onDisconnect(AsyncMqttClientDisconnectReason reason) {
    status = ConnectionStatus::DISCONNECTED;
    
    Serial.printf("MQTT Disconnected, reason: %d\n", reason);
    
    if (reason == AsyncMqttClientDisconnectReason::TCP_DISCONNECTED) {
        Serial.println("MQTT: TCP disconnected, will retry...");
    } else if (reason == AsyncMqttClientDisconnectReason::MQTT_UNACCEPTABLE_PROTOCOL_VERSION) {
        Serial.println("MQTT: Unacceptable protocol version");
    } else if (reason == AsyncMqttClientDisconnectReason::MQTT_IDENTIFIER_REJECTED) {
        Serial.println("MQTT: Identifier rejected");
    } else if (reason == AsyncMqttClientDisconnectReason::MQTT_SERVER_UNAVAILABLE) {
        Serial.println("MQTT: Server unavailable");
    } else if (reason == AsyncMqttClientDisconnectReason::MQTT_MALFORMED_CREDENTIALS) {
        Serial.println("MQTT: Malformed credentials");
    } else if (reason == AsyncMqttClientDisconnectReason::MQTT_NOT_AUTHORIZED) {
        Serial.println("MQTT: Not authorized");
    }
}

void MQTTInterface::onMessage(char* topic, char* payload, 
                               AsyncMqttClientMessageProperties properties,
                               size_t len, size_t index, size_t total) {
    // Create null-terminated string from payload
    String message;
    message.reserve(len);
    for (size_t i = 0; i < len; i++) {
        message += payload[i];
    }
    
    String topicStr(topic);
    
    Serial.printf("MQTT Message: [%s] %s\n", topic, message.c_str());
    
    // Handle command topic
    if (topicStr == getCommandTopic()) {
        handleCommand(message);
    }
}

void MQTTInterface::onPublish(uint16_t packetId) {
    // Packet published successfully
}

// ============================================================================
// Message Handlers
// ============================================================================

void MQTTInterface::handleCommand(const String& payload) {
    String cmd = payload;
    cmd.toUpperCase();
    cmd.trim();
    
    Serial.printf("MQTT Command: %s\n", cmd.c_str());
    
    if (cmd == "START" || cmd == "ON") {
        if (startCallback) {
            startCallback();
        }
    } else if (cmd == "STOP" || cmd == "OFF") {
        if (stopCallback) {
            stopCallback();
        }
    } else if (cmd == "PAUSE") {
        if (pauseCallback) {
            pauseCallback();
        }
    } else if (cmd == "RESUME") {
        if (resumeCallback) {
            resumeCallback();
        }
    } else {
        Serial.printf("Unknown MQTT command: %s\n", cmd.c_str());
    }
    
    // Publish updated state
    publishState();
}
