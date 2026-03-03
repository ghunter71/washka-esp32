# Design Document

## Overview

Система управления посудомоечной машиной на базе ESP32 представляет собой встраиваемое решение с веб-интерфейсом, REST API и Telegram-ботом. Архитектура построена на принципах разделения ответственности с четким разграничением между уровнями управления оборудованием, бизнес-логикой и пользовательскими интерфейсами.

Ключевые технологии:
- ESP32 (WROOM-32) с FreeRTOS
- AsyncWebServer для неблокирующего веб-сервера
- Preferences API для NVS (Non-Volatile Storage)
- UniversalTelegramBot для интеграции с Telegram
- ArduinoJson для сериализации данных
- AsyncElegantOTA для беспроводных обновлений

## Architecture

### High-Level Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     User Interfaces                          │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │ Web Interface│  │   REST API   │  │ Telegram Bot │      │
│  │  (Bootstrap) │  │    (JSON)    │  │   (Commands) │      │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘      │
└─────────┼──────────────────┼──────────────────┼─────────────┘
          │                  │                  │
          └──────────────────┼──────────────────┘
                             │
┌────────────────────────────┼─────────────────────────────────┐
│                    Application Layer                          │
│  ┌──────────────────────────────────────────────────────┐   │
│  │              State Machine Controller                 │   │
│  │  (Wash Cycles, State Persistence, Recovery)          │   │
│  └────────┬─────────────────────────────────────────────┘   │
│           │                                                   │
│  ┌────────┴─────────┬──────────────┬──────────────┐        │
│  │  WaterControl    │ ActuatorMgr  │ ConfigMgr    │        │
│  │  (Gerkon logic)  │ (Safety)     │ (NVS)        │        │
│  └────────┬─────────┴──────┬───────┴──────┬───────┘        │
└───────────┼─────────────────┼──────────────┼────────────────┘
            │                 │              │
┌───────────┼─────────────────┼──────────────┼────────────────┐
│           │    Hardware Abstraction Layer  │                 │
│  ┌────────┴─────────┐  ┌───┴──────────┐  ┌┴──────────┐    │
│  │   GPIO Manager   │  │  WiFi Manager│  │ OTA Update │    │
│  │  (Pin mapping)   │  │  (AP/STA)    │  │  (Secure)  │    │
│  └──────────────────┘  └──────────────┘  └────────────┘    │
└─────────────────────────────────────────────────────────────┘
```

### Component Interaction Flow

1. **Startup Sequence:**
   - Load configuration from NVS
   - Initialize GPIO with configured pins
   - Attempt WiFi connection (fallback to AP mode)
   - Start web server, API endpoints, and Telegram bot

2. **Normal Operation:**
   - State machine controls wash cycle progression
   - Sensors monitored via interrupts and polling
   - Actuators controlled through safety-checked interface
   - State persisted at each transition
   - UI updates pushed via WebSocket

3. **Configuration Changes:**
   - User submits changes via Web/API/Telegram
   - Validation performed
   - Settings saved to NVS
   - System applies changes (may require restart)

## Components and Interfaces

### 1. ConfigManager

**Responsibility:** Управление всеми настройками системы с сохранением в NVS.

**Interface:**
```cpp
class ConfigManager {
public:
    bool begin();
    
    // WiFi settings
    String getWiFiSSID();
    String getWiFiPassword();
    void setWiFiCredentials(const String& ssid, const String& password);
    
    // GPIO configuration
    struct PinConfig {
        uint8_t washengine;
        uint8_t pompa;
        uint8_t watergerkon;
        uint8_t powder;
        uint8_t water_valve;
        uint8_t button;
        uint8_t led;
    };
    PinConfig getPinConfig();
    bool setPinConfig(const PinConfig& config);
    
    // Timing configuration
    struct TimingConfig {
        unsigned long tpomp;
        unsigned long washtime0;
        unsigned long washtime1;
        unsigned long washtime2;
        unsigned long washtime3;
        unsigned long pausa;
        unsigned long water_in_timer;
    };
    TimingConfig getTimingConfig();
    bool setTimingConfig(const TimingConfig& config);
    
    // Gerkon configuration
    uint16_t getGerkonThreshold();
    void setGerkonThreshold(uint16_t threshold);
    uint16_t getGerkonDebounceMs();
    void setGerkonDebounceMs(uint16_t ms);
    
    // Telegram configuration
    String getTelegramToken();
    void setTelegramToken(const String& token);
    std::vector<int64_t> getAllowedChatIds();
    void addAllowedChatId(int64_t chatId);
    
    // Factory reset
    void factoryReset();
    
    // Export/Import
    String exportToJson();
    bool importFromJson(const String& json);
};
```

### 2. StateManager

**Responsibility:** Управление состоянием машины и восстановление после сбоев.

**Interface:**
```cpp
enum class WashState : uint8_t {
    IDLE = 0,
    DRAIN_PREWASH = 2,
    FILL_PREWASH = 3,
    PREWASH = 4,
    DRAIN_WASH = 5,
    FILL_WASH = 6,
    WASH = 7,
    DRAIN_RINSE1 = 8,
    FILL_RINSE1 = 9,
    RINSE1 = 10,
    DRAIN_RINSE2 = 11,
    FILL_RINSE2 = 12,
    RINSE2 = 13,
    FINAL_DRAIN = 14,
    COMPLETE = 15,
    ERROR = 255
};

class StateManager {
public:
    bool begin();
    
    WashState getCurrentState();
    void setState(WashState newState);
    
    bool startCycle();
    bool stopCycle();
    bool pauseCycle();
    bool resumeCycle();
    
    // Recovery

    
    // State info
    String getStateDescription();
    uint8_t getProgressPercent();
    unsigned long getElapsedTime();
    unsigned long getEstimatedRemaining();
    
    // Callbacks
    typedef std::function<void(WashState, WashState)> StateChangeCallback;
    void onStateChange(StateChangeCallback callback);
};
```

### 3. WaterControl

**Responsibility:** Контроль набора воды с использованием геркона.

**Interface:**
```cpp
class WaterControl {
public:
    bool begin(uint8_t gerkonPin);
    
    enum class FillResult {
        SUCCESS_GERKON,
        SUCCESS_TIMEOUT,
        ERROR_TIMEOUT,
        ERROR_SENSOR
    };
    
    FillResult fillWater(uint16_t gerkonThreshold, unsigned long timeoutMs);
    
    uint16_t getGerkonCount();
    void resetGerkonCount();
    
    // Debouncing
    void setDebounceMs(uint16_t ms);
    
    // Callbacks for monitoring
    typedef std::function<void(uint16_t count)> GerkonTriggerCallback;
    void onGerkonTrigger(GerkonTriggerCallback callback);
};
```

### 3a. ButtonControl

**Responsibility:** Надежное определение нажатий кнопки с защитой от дребезга.

**Interface:**
```cpp
class ButtonControl {
public:
    bool begin(uint8_t buttonPin);
    
    enum class PressType {
        NONE,
        SHORT_PRESS,
        LONG_PRESS
    };
    
    // Check for button press (call in loop)
    PressType checkPress();
    
    // Configuration
    void setDebounceMs(uint16_t ms);
    void setLongPressMs(unsigned long ms);
    
    // State query
    bool isPressed();
    unsigned long getPressedDuration();
    
    // Callbacks
    typedef std::function<void(PressType)> ButtonPressCallback;
    void onButtonPress(ButtonPressCallback callback);
    
private:
    enum class ButtonState {
        IDLE_HIGH,
        PRESSED_LOW,
        RELEASED_HIGH
    };
    
    ButtonState currentState;
    unsigned long lastTransitionTime;
    unsigned long pressStartTime;
};
```

### 4. ActuatorManager

**Responsibility:** Безопасное управление актуаторами с проверкой взаимоисключающих состояний.

**Interface:**
```cpp
class ActuatorManager {
public:
    bool begin(const ConfigManager::PinConfig& pins);
    
    // Individual actuator control
    bool setWashEngine(bool state);
    bool setPump(bool state);
    bool setWaterValve(bool state);
    bool setPowderDispenser(bool state);
    bool setLED(bool state);
    
    // Safety checks
    bool canFillWater();  // Check pump is off
    bool canDrain();      // Check valve is off
    
    // Emergency stop
    void emergencyStop();
    
    // Status
    struct ActuatorStatus {
        bool washengine;
        bool pompa;
        bool water_valve;
        bool powder;
        bool led;
    };
    ActuatorStatus getStatus();
};
```

### 5. WebInterface

**Responsibility:** Предоставление веб-интерфейса на Bootstrap с темной темой.

**Interface:**
```cpp
class WebInterface {
public:
    bool begin(AsyncWebServer* server);
    
    // Serve pages
    void handleRoot(AsyncWebServerRequest* request);
    void handleConfig(AsyncWebServerRequest* request);
    void handleControl(AsyncWebServerRequest* request);
    void handleStatus(AsyncWebServerRequest* request);
    
    // WebSocket for real-time updates
    void broadcastStatus();
    
    // Static resources (embedded)
    void serveBootstrapCSS(AsyncWebServerRequest* request);
    void serveBootstrapJS(AsyncWebServerRequest* request);
};
```

### 6. RestAPI

**Responsibility:** REST API для программного управления.

**Interface:**
```cpp
class RestAPI {
public:
    bool begin(AsyncWebServer* server);
    
    // Endpoints:
    // GET  /api/status          - Current system status
    // GET  /api/config          - Current configuration
    // POST /api/config          - Update configuration
    // POST /api/control/start   - Start wash cycle
    // POST /api/control/stop    - Stop wash cycle
    // POST /api/control/pause   - Pause wash cycle
    // GET  /api/docs            - OpenAPI documentation
    
private:
    void handleGetStatus(AsyncWebServerRequest* request);
    void handleGetConfig(AsyncWebServerRequest* request);
    void handlePostConfig(AsyncWebServerRequest* request);
    void handleControlStart(AsyncWebServerRequest* request);
    void handleControlStop(AsyncWebServerRequest* request);
    void handleDocs(AsyncWebServerRequest* request);
};
```

### 7. TelegramInterface

**Responsibility:** Управление через Telegram-бот.

**Interface:**
```cpp
class TelegramInterface {
public:
    bool begin(const String& token);
    
    void loop();  // Call in main loop for polling
    
    // Commands:
    // /start    - Show available commands
    // /status   - Get current status
    // /startwash - Start wash cycle
    // /stop     - Stop wash cycle
    // /config   - Show current config
    
    // Notifications
    void notifyStateChange(WashState oldState, WashState newState);
    void notifyError(const String& error);
    void notifyComplete();
    
private:
    bool isAuthorized(int64_t chatId);
    void handleMessage(const String& text, int64_t chatId);
};
```

### 8. WiFiManager

**Responsibility:** Управление WiFi-подключением с поддержкой AP-режима.

**Interface:**
```cpp
class WiFiManager {
public:
    bool begin();
    
    enum class ConnectionStatus {
        CONNECTED,
        CONNECTING,
        AP_MODE,
        FAILED
    };
    
    ConnectionStatus getStatus();
    String getIPAddress();
    String getAPSSID();
    
    // Captive portal for configuration
    void startConfigPortal();
    void stopConfigPortal();
    
private:
    void handleWiFiConfig(AsyncWebServerRequest* request);
};
```

### 9. OTAUpdater

**Responsibility:** Безопасное обновление прошивки по воздуху.

**Interface:**
```cpp
class OTAUpdater {
public:
    bool begin(AsyncWebServer* server);
    
    // AsyncElegantOTA integration
    void handleUpdate();
    
    // Safety checks
    bool canUpdate();  // Check if system is idle
    
    // Callbacks
    typedef std::function<void(uint8_t progress)> ProgressCallback;
    void onProgress(ProgressCallback callback);
};
```

### 10. DebugLogger

**Responsibility:** Централизованное логирование в Serial и WebSerial.

**Interface:**
```cpp
class DebugLogger {
public:
    bool begin(AsyncWebServer* server);
    
    enum class LogLevel {
        DEBUG,
        INFO,
        WARNING,
        ERROR
    };
    
    void log(LogLevel level, const String& message);
    void debug(const String& message);
    void info(const String& message);
    void warning(const String& message);
    void error(const String& message);
    
    // DEBUG command handler
    void dumpSystemState();
    
private:
    void formatMessage(LogLevel level, const String& message);
};
```

## Data Models

### Configuration Storage (NVS)

```cpp
// NVS Namespace: "washka"
// Keys:
namespace NVSKeys {
    // WiFi
    constexpr char WIFI_SSID[] = "wifi_ssid";
    constexpr char WIFI_PASS[] = "wifi_pass";
    
    // GPIO Pins
    constexpr char PIN_WASHENGINE[] = "pin_wash";
    constexpr char PIN_POMPA[] = "pin_pompa";
    constexpr char PIN_GERKON[] = "pin_gerkon";
    constexpr char PIN_POWDER[] = "pin_powder";
    constexpr char PIN_VALVE[] = "pin_valve";
    constexpr char PIN_BUTTON[] = "pin_button";
    constexpr char PIN_LED[] = "pin_led";
    
    // Timing (in milliseconds)
    constexpr char TIME_PUMP[] = "time_pump";
    constexpr char TIME_PREWASH[] = "time_prewash";
    constexpr char TIME_WASH[] = "time_wash";
    constexpr char TIME_RINSE1[] = "time_rinse1";
    constexpr char TIME_RINSE2[] = "time_rinse2";
    constexpr char TIME_PAUSE[] = "time_pause";
    constexpr char TIME_WATER_TIMEOUT[] = "time_water";
    
    // Gerkon
    constexpr char GERKON_THRESHOLD[] = "gerk_thresh";
    constexpr char GERKON_DEBOUNCE[] = "gerk_debounce";
    
    // Telegram
    constexpr char TG_TOKEN[] = "tg_token";
    constexpr char TG_CHAT_IDS[] = "tg_chats";
}
```

### JSON API Response Format

```json
{
  "status": {
    "state": "WASH",
    "stateDescription": "Washing in progress",
    "progress": 45,
    "elapsedTime": 1200000,
    "estimatedRemaining": 1440000
  },
  "sensors": {
    "gerkonCount": 15,
    "gerkonState": false
  },
  "actuators": {
    "washengine": true,
    "pompa": false,
    "waterValve": false,
    "powder": false,
    "led": true
  },
  "system": {
    "uptime": 3600000,
    "freeHeap": 180000,
    "wifiRSSI": -65,
    "ipAddress": "192.168.1.100"
  }
}
```

### Pin Legend Data Structure

```cpp
struct PinInfo {
    uint8_t gpio;
    String label;        // e.g., "D32"
    String description;  // e.g., "ADC1_4/GPIO32/pin12"
    bool canUse;         // false for strapping/flash pins
    String warning;      // e.g., "Strapping pin - use with caution"
};

// WROOM-32 Pin Map
const PinInfo WROOM32_PINS[] = {
    {32, "D32", "ADC1_4/GPIO32/pin12", true, ""},
    {33, "D33", "ADC1_5/GPIO33/pin13", true, ""},
    {25, "D25", "ADC2_8/GPIO25/pin14", true, ""},
    {26, "D26", "ADC2_9/GPIO26/pin15", true, ""},
    {27, "D27", "ADC2_7/GPIO27/pin16", true, ""},
    {14, "D14", "ADC2_6/GPIO14/pin17", true, ""},
    {12, "D12", "ADC2_5/GPIO12/pin18", false, "Strapping pin - boot fails if HIGH"},
    {13, "D13", "ADC2_4/GPIO13/pin20", true, ""},
    // ... more pins
    {0, "D0", "GPIO0/pin25", false, "Strapping pin - boot mode"},
    {2, "D2", "GPIO2/pin24", false, "Strapping pin - boot mode"},
    {15, "D15", "ADC2_3/GPIO15/pin23", false, "Strapping pin - boot fails if HIGH"},
    // ... complete map
};
```

## Correctness Properties


*A property is a characteristic or behavior that should hold true across all valid executions of a system—essentially, a formal statement about what the system should do. Properties serve as the bridge between human-readable specifications and machine-verifiable correctness guarantees.*

### Property 1: ESP32 pin validation
*For any* GPIO pin configuration, all assigned pins must be within valid ESP32 GPIO ranges (0-39) and not assigned to multiple functions
**Validates: Requirements 1.2, 3.2**

### Property 2: WiFi credentials persistence round-trip
*For any* valid WiFi credentials (SSID and password), saving to NVS then loading should return identical values
**Validates: Requirements 2.3**

### Property 3: GPIO configuration persistence round-trip
*For any* valid pin configuration, saving to NVS, restarting, and loading should return the same pin assignments
**Validates: Requirements 3.3, 3.4**

### Property 4: Timing configuration validation
*For any* timing configuration values, the system should reject values outside acceptable ranges (e.g., negative values, values exceeding maximum safe duration)
**Validates: Requirements 4.2, 4.5**

### Property 5: Timing configuration persistence round-trip
*For any* valid timing configuration, saving to NVS then loading should return identical values
**Validates: Requirements 4.3, 4.4**

### Property 6: State transition safety
*For any* valid system state, issuing a start command should transition to the next appropriate state in the wash cycle sequence
**Validates: Requirements 6.2**

### Property 7: Stop command safety
*For any* running state, issuing a stop command should halt all actuators and persist the current state to NVS
**Validates: Requirements 6.3**

### Property 8: API JSON response validity
*For any* API endpoint response, the returned data must be valid JSON and include appropriate HTTP status codes
**Validates: Requirements 7.2**

### Property 9: API authentication enforcement
*For any* API request without valid authentication credentials, the system should return 401 Unauthorized status
**Validates: Requirements 7.3**

### Property 10: API parameter validation
*For any* control command with invalid parameters, the API should reject the request with 400 Bad Request and error details
**Validates: Requirements 7.5**

### Property 11: Telegram authorization enforcement
*For any* Telegram command from a chat ID not in the allowed list, the bot should reject the command with an authorization error message
**Validates: Requirements 8.3**

### Property 12: Telegram state change notifications
*For any* state transition in the wash cycle, the system should send notifications to all subscribed Telegram chat IDs
**Validates: Requirements 8.4**

### Property 13: Invalid pin warning
*For any* pin selection that includes strapping pins or flash pins, the system should display a warning about limitations
**Validates: Requirements 9.5**

### Property 14: Gerkon counter increment
*For any* gerkon state transition from LOW to HIGH (after debounce period), the water control counter should increment by exactly one
**Validates: Requirements 10.2**

### Property 15: Gerkon threshold trigger
*For any* configured gerkon threshold value, when the counter reaches that threshold, the water valve should close immediately
**Validates: Requirements 10.3**

### Property 16: Gerkon debouncing
*For any* sequence of gerkon triggers occurring within the debounce time window, only the first trigger should increment the counter
**Validates: Requirements 10.5**

### Property 17: OTA signature validation
*For any* firmware update without a valid signature, the OTA updater should reject the update before applying
**Validates: Requirements 11.1**

### Property 18: OTA critical state blocking
*For any* system state marked as critical (e.g., WASH, DRAIN), OTA update initiation should be blocked
**Validates: Requirements 11.5**

### Property 19: Log timestamp presence
*For any* log message output by the system, it must include a timestamp in the format [HH:MM:SS.mmm]
**Validates: Requirements 12.1**

### Property 20: Error logging completeness
*For any* error condition detected by the system, an error-level log entry must be created with error details
**Validates: Requirements 12.4**

### Property 21: Configuration NVS persistence
*For any* configuration change (WiFi, GPIO, timing, gerkon, Telegram), the new values must be saved to NVS immediately
**Validates: Requirements 13.1**

### Property 22: Configuration export/import round-trip
*For any* system configuration, exporting to JSON then importing should restore identical configuration values
**Validates: Requirements 13.5**

### Property 23: Actuator mutual exclusion
*For any* actuator state combination, the system should prevent simultaneous activation of water valve and pump (fill and drain)
**Validates: Requirements 15.1**

### Property 24: Timeout safety shutdown
*For any* operation timeout event, all actuators (pump, valve, wash engine, powder) should be set to OFF state
**Validates: Requirements 15.2**

### Property 25: Emergency stop completeness
*For any* system state, triggering emergency stop should immediately set all actuators to OFF and transition to ERROR state
**Validates: Requirements 15.3**

### Property 26: Invalid sensor halt
*For any* sensor reading that fails validation (e.g., gerkon stuck, impossible values), the system should halt operations and enter ERROR state
**Validates: Requirements 15.4**

### Property 27: Button press detection sequence
*For any* button pin state sequence HIGH → LOW → HIGH (with debounce), the system should register exactly one button press event
**Validates: Requirements 16.1**

### Property 28: Button debounce filtering
*For any* sequence of button state changes occurring within the debounce time window, only the complete HIGH → LOW → HIGH sequence after debounce should register as a press
**Validates: Requirements 16.2, 16.5**

### Property 29: Button action execution
*For any* detected button press event, the system should execute exactly one configured action (start, stop, or toggle)
**Validates: Requirements 16.3**

### Property 30: Button long press distinction
*For any* button press held longer than the long press threshold, the system should distinguish it from a short press and execute the appropriate action
**Validates: Requirements 16.4**

## Error Handling

### Error Categories

1. **Configuration Errors**
   - Invalid pin assignments (duplicates, out of range)
   - Invalid timing values (negative, too large)
   - Corrupted NVS data
   - **Handling:** Reject changes, log error, use defaults, notify user

2. **Hardware Errors**
   - Sensor failures (gerkon stuck, no response)
   - Actuator failures (no feedback)
   - GPIO errors
   - **Handling:** Enter ERROR state, disable actuators, log details, notify via Telegram

3. **Network Errors**
   - WiFi connection loss
   - Telegram API unavailable
   - HTTP request failures
   - **Handling:** Retry with exponential backoff, continue operation in degraded mode, log warnings

4. **State Errors**
   - Invalid state transitions
   - Corrupted state in NVS
   - Timeout during operations
   - **Handling:** Transition to safe state (IDLE or ERROR), log recovery, notify user

5. **OTA Errors**
   - Invalid firmware signature
   - Update during critical operation
   - Insufficient space
   - **Handling:** Reject update, log reason, notify user, maintain current firmware

### Error Recovery Strategy

```cpp
enum class ErrorSeverity {
    WARNING,    // Log and continue
    RECOVERABLE, // Attempt recovery, may degrade functionality
    CRITICAL    // Enter ERROR state, disable actuators, require user intervention
};

class ErrorHandler {
public:
    void handleError(ErrorSeverity severity, const String& errorCode, const String& details);
    
private:
    void logError(const String& errorCode, const String& details);
    void notifyUser(const String& message);
    void enterSafeState();
};
```

### Watchdog Configuration

- **Watchdog Timeout:** 120 seconds (same as original)
- **Feed Points:** 
  - Main loop iteration
  - Before long-running operations
  - During state transitions
  - In blocking waits (with yield())

## Testing Strategy

### Unit Testing

**Framework:** PlatformIO Unity Test Framework

**Test Coverage:**
- ConfigManager: NVS read/write, validation, defaults
- StateManager: State transitions, persistence, recovery
- WaterControl: Gerkon counting, debouncing, timeout
- ButtonControl: Press detection, debouncing, long press
- ActuatorManager: Safety checks, mutual exclusion
- Pin validation: Range checks, duplicate detection
- JSON serialization: API responses, config export/import

**Example Unit Tests:**
```cpp
void test_config_pin_validation() {
    ConfigManager config;
    ConfigManager::PinConfig pins;
    
    // Test duplicate detection
    pins.washengine = 5;
    pins.pompa = 5;  // Duplicate
    TEST_ASSERT_FALSE(config.setPinConfig(pins));
    
    // Test valid configuration
    pins.pompa = 4;
    TEST_ASSERT_TRUE(config.setPinConfig(pins));
}

void test_state_initialization() {
    StateManager state;
    // After initialization, state should always be IDLE
    TEST_ASSERT_EQUAL(WashState::IDLE, state.getCurrentState());
}
```

### Property-Based Testing

**Framework:** RapidCheck for C++ (or custom implementation with random generators)

**Configuration:** Minimum 100 iterations per property test

**Property Test Structure:**
```cpp
// Example property test
void property_test_pin_no_duplicates() {
    for (int i = 0; i < 100; i++) {
        // Generate random pin configuration
        ConfigManager::PinConfig pins = generateRandomPinConfig();
        
        // Property: If config is accepted, no pins should be duplicated
        if (configManager.setPinConfig(pins)) {
            std::set<uint8_t> uniquePins = {
                pins.washengine, pins.pompa, pins.watergerkon,
                pins.powder, pins.water_valve, pins.button, pins.led
            };
            TEST_ASSERT_EQUAL(7, uniquePins.size());
        }
    }
}
```

**Property Test Tags:**
Each property-based test must include a comment tag in this format:
```cpp
// **Feature: esp32-washka-upgrade, Property 1: ESP32 pin validation**
// **Validates: Requirements 1.2, 3.2**
void property_test_esp32_pin_validation() {
    // Test implementation
}
```

### Integration Testing

**Test Scenarios:**
1. Full wash cycle execution (mocked hardware)
2. WiFi configuration flow (AP mode → credentials → connection)
3. System startup always in IDLE state
4. API endpoint integration (request → processing → response)
5. Telegram command flow (command → validation → execution → response)

### Hardware-in-Loop Testing

**Manual Test Cases:**
1. Gerkon counting accuracy with real sensor
2. Actuator timing verification
3. WiFi range and stability
4. OTA update in real conditions
5. WebSerial performance under load

### Test Utilities

```cpp
// Mock hardware for testing
class MockGPIO {
public:
    void digitalWrite(uint8_t pin, uint8_t value);
    uint8_t digitalRead(uint8_t pin);
    void simulateGerkonTrigger();
};

// Random generators for property tests
ConfigManager::PinConfig generateRandomPinConfig();
ConfigManager::TimingConfig generateRandomTimingConfig();
WashState generateRandomWashState();
```

## Implementation Notes

### ESP32-Specific Considerations

1. **Pin Restrictions:**
   - GPIO 6-11: Connected to flash, cannot use
   - GPIO 0, 2, 15: Strapping pins, use with caution
   - GPIO 34-39: Input only, no pull-up/pull-down
   - ADC2 pins: Cannot use when WiFi active

2. **Memory Management:**
   - Use PSRAM if available for large buffers
   - Monitor heap fragmentation
   - Use static allocation for critical components

3. **Task Management:**
   - Main loop on core 1
   - WiFi/network tasks on core 0 (default)
   - Use FreeRTOS tasks for concurrent operations

4. **NVS Partitioning:**
   - Reserve adequate NVS partition size (minimum 16KB)
   - Use separate namespaces for different components
   - Implement wear leveling awareness

### Security Considerations

1. **WiFi Credentials:**
   - Store encrypted in NVS
   - Never expose in logs or API responses
   - Clear from memory after use

2. **API Authentication:**
   - Use API keys stored in NVS
   - Implement rate limiting
   - Log authentication failures

3. **Telegram Bot:**
   - Validate chat IDs against whitelist
   - Never expose bot token
   - Implement command rate limiting

4. **OTA Updates:**
   - Verify firmware signatures
   - Use HTTPS for update downloads
   - Implement rollback mechanism

### Performance Targets

- **Boot Time:** < 5 seconds to operational
- **Web Interface Load:** < 2 seconds
- **API Response Time:** < 200ms
- **State Persistence:** < 50ms
- **Gerkon Response:** < 10ms (after debounce)
- **Telegram Command Response:** < 1 second

### Bootstrap Integration

**Embedded Resources:**
- Bootstrap 5.3 CSS (minified) - ~25KB gzipped
- Bootstrap 5.3 JS (minified) - ~15KB gzipped
- Custom dark theme CSS - ~2KB
- Icons: Bootstrap Icons subset - ~5KB

**Storage:** Use SPIFFS or LittleFS for static files

**Optimization:**
- Serve with gzip compression
- Use browser caching headers
- Minify HTML templates

### WebSocket Protocol

**Status Updates:**
```json
{
  "type": "status",
  "timestamp": 1234567890,
  "state": "WASH",
  "progress": 45,
  "actuators": {
    "washengine": true,
    "pompa": false,
    "waterValve": false
  },
  "sensors": {
    "gerkonCount": 15
  }
}
```

**Update Frequency:** 1 Hz during operation, 0.1 Hz when idle

## Deployment

### Build Configuration

```ini
[env:esp32]
platform = espressif32
board = esp32dev
framework = arduino
lib_deps = 
    me-no-dev/AsyncTCP
    me-no-dev/ESPAsyncWebServer
    bblanchon/ArduinoJson
    witnessmenow/UniversalTelegramBot
    ayushsharma82/AsyncElegantOTA
monitor_speed = 115200
board_build.partitions = partitions.csv
```

### Partition Table

```csv
# Name,   Type, SubType, Offset,  Size
nvs,      data, nvs,     0x9000,  0x5000
otadata,  data, ota,     0xe000,  0x2000
app0,     app,  ota_0,   0x10000, 0x180000
app1,     app,  ota_1,   0x190000,0x180000
spiffs,   data, spiffs,  0x310000,0xF0000
```

### Initial Setup Procedure

1. Flash firmware via USB
2. Device starts in AP mode (SSID: "Washka-Setup")
3. Connect to AP and configure WiFi
4. Device reboots and connects to WiFi
5. Access web interface at device IP
6. Configure GPIO pins and timing
7. Configure Telegram bot token (optional)
8. Test individual actuators
9. Run first wash cycle

### Maintenance

- **Logs:** Accessible via WebSerial at /webserial
- **Updates:** Via web interface at /update
- **Backup:** Export configuration via API or web interface
- **Factory Reset:** Hold button for 10 seconds during boot
