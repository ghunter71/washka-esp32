/**
 * MQTTInterface.h
 * 
 * MQTT integration for Home Assistant and IoT platforms.
 * Supports discovery, state publishing, and command receiving.
 * 
 * @version 1.0.0
 */

#ifndef MQTT_INTERFACE_H
#define MQTT_INTERFACE_H

#include <Arduino.h>
#include <functional>
#include <vector>

// Forward declarations
class AsyncMqttClient;
class StateManager;
class ActuatorManager;
class WaterControl;
class ConfigManager;

class MQTTInterface {
public:
    // MQTT Configuration
    struct MQTTConfig {
        String server;
        uint16_t port;
        String username;
        String password;
        String clientId;
        String topicPrefix;
        bool enabled;
        bool discoveryEnabled;
        unsigned long keepAlive;
        
        MQTTConfig() :
            server(""),
            port(1883),
            username(""),
            password(""),
            clientId("washka-esp32"),
            topicPrefix("homeassistant"),
            enabled(false),
            discoveryEnabled(true),
            keepAlive(60) {}
    };
    
    // Connection status
    enum class ConnectionStatus {
        DISCONNECTED,
        CONNECTING,
        CONNECTED,
        ERROR
    };
    
    // Command callback types
    typedef std::function<void()> CommandCallback;
    typedef std::function<void(int)> ScheduledStartCallback;
    
    MQTTInterface();
    ~MQTTInterface();
    
    // Initialization
    bool begin(ConfigManager* config, StateManager* state, 
               ActuatorManager* actuator, WaterControl* water);
    
    // Configuration
    void setConfig(const MQTTConfig& config);
    MQTTConfig getConfig();
    bool saveConfig();
    bool loadConfig();
    
    // Connection management
    void connect();
    void disconnect();
    ConnectionStatus getStatus();
    bool isConnected();
    
    // Main loop - call from main loop
    void loop();
    
    // State publishing
    void publishState();
    void publishStatus();
    void publishAvailability(bool online = true);
    void publishSensorData();
    
    // Home Assistant discovery
    void publishDiscovery();
    void clearDiscovery();
    
    // Command registration
    void onStartCommand(CommandCallback callback);
    void onStopCommand(CommandCallback callback);
    void onPauseCommand(CommandCallback callback);
    void onResumeCommand(CommandCallback callback);
    
private:
    AsyncMqttClient* mqttClient;
    ConfigManager* configManager;
    StateManager* stateManager;
    ActuatorManager* actuatorManager;
    WaterControl* waterControl;
    
    MQTTConfig mqttConfig;
    ConnectionStatus status;
    
    // Callbacks
    CommandCallback startCallback;
    CommandCallback stopCallback;
    CommandCallback pauseCallback;
    CommandCallback resumeCallback;
    
    // Timing
    unsigned long lastPublishTime;
    unsigned long lastReconnectAttempt;
    static const unsigned long PUBLISH_INTERVAL = 30000;  // 30 seconds
    static const unsigned long RECONNECT_INTERVAL = 5000; // 5 seconds
    
    // Topic helpers
    String getTopic(const String& suffix);
    String getStateTopic();
    String getCommandTopic();
    String getAvailabilityTopic();
    String getConfigTopic(const String& component, const String& objectId);
    
    // MQTT callbacks
    void onConnect(bool sessionPresent);
    void onDisconnect(AsyncMqttClientDisconnectReason reason);
    void onMessage(char* topic, char* payload, 
                   AsyncMqttClientMessageProperties properties,
                   size_t len, size_t index, size_t total);
    void onPublish(uint16_t packetId);
    
    // Message handlers
    void handleCommand(const String& payload);
    
    // Discovery helpers
    void publishSwitchDiscovery();
    void publishSensorDiscovery();
    void publishBinarySensorDiscovery();
};

#endif // MQTT_INTERFACE_H
