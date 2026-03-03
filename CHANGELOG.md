# Washka ESP32 v2.0 - Changelog

## Overview

This release represents a major refactoring and enhancement of the Washka ESP32 dishwasher control system, addressing critical security issues, architectural improvements, and adding significant new features.

## Critical Fixes

### Security (CRITICAL-001, CRITICAL-002, CRITICAL-003)

**AuthMiddleware Component** (`AuthMiddleware.h/cpp`)
- Token-based authentication for REST API
- Three permission levels: Viewer, User, Admin
- Bearer token authentication via headers
- Secure token storage in NVS
- Rate limiting support for failed authentication attempts
- Tokens: `X-API-Key` header, `Authorization: Bearer <token>`, or URL parameter `?token=`

### Race Conditions (HIGH-001)

**StateManager Refactoring** (`StateManager.h/cpp`)
- FreeRTOS mutex protection for all state transitions
- Async callback queue for non-blocking notifications
- `processCallbacks()` method to be called from main loop
- State persistence for recovery after reboot
- Proper `isValidTransition()` enforcement

### Error Handling (ERROR-001, ERROR-002)

**ApplicationKernel** (`ApplicationKernel.h/cpp`)
- Centralized error reporting with `reportError()`
- Safe mode for critical failures
- Graceful degradation when components fail
- Proper initialization sequence with dependency checking
- Automatic actuator emergency stop on critical errors

## Architectural Improvements

### Dependency Injection (ARCH-001)

**ApplicationKernel Pattern**
- All components managed via `unique_ptr`
- Explicit initialization order
- Clear lifecycle management
- Single access point for all services
- Easy mocking for unit tests

### FreeRTOS Tasks (PERF-001)

- Async callback processing in StateManager
- Non-blocking state notifications
- Event queue for state changes
- Proper task isolation

## New Features

### MQTT Integration (Home Assistant)

**MQTTInterface Component** (`MQTTInterface.h/cpp`)
- Full Home Assistant MQTT Discovery support
- Automatic device registration
- State publishing every 30 seconds
- Command subscription (START, STOP, PAUSE, RESUME)
- Last Will Testament (LWT) for availability
- Topics:
  - `[prefix]/washka/state` - Current state
  - `[prefix]/washka/command` - Commands
  - `[prefix]/washka/availability` - Online/Offline
  - `[prefix]/washka/sensors` - Sensor data

### Schedule Management

**ScheduleManager Component** (`ScheduleManager.h/cpp`)
- Delayed start (by minutes or specific time)
- Recurring schedules (daily, weekly)
- NTP time synchronization
- Schedule persistence in NVS
- Multiple schedule support

### Statistics & Analytics

- Total cycle count
- Total runtime tracking
- Average cycle time
- Water consumption tracking (via gerkon count)
- Cycle history with timestamps
- Export to CSV

### Setup Wizard

**Web-based Configuration** (`setup-wizard.html`)
- 5-step guided setup process
- WiFi configuration with network scanning
- Security token generation
- Notification setup (Telegram, MQTT)
- Configuration summary

## New Web Interface Pages

1. **config-mqtt.html** - MQTT broker configuration
2. **schedule.html** - Schedule management and delayed start
3. **statistics.html** - Usage analytics and cycle history
4. **setup-wizard.html** - Initial setup wizard

## API Endpoints (New)

### Authentication
- `POST /api/auth/config` - Configure authentication
- `GET /api/auth/config` - Get current auth settings
- `POST /api/auth/token` - Generate new token

### MQTT
- `GET /api/mqtt/config` - Get MQTT configuration
- `POST /api/mqtt/config` - Save MQTT configuration
- `GET /api/mqtt/status` - Connection status
- `POST /api/mqtt/test` - Test connection

### Schedule
- `GET /api/schedule/time` - Current time and sync status
- `GET /api/schedule/list` - List all schedules
- `POST /api/schedule` - Create schedule
- `PUT /api/schedule/:id` - Update schedule
- `DELETE /api/schedule/:id` - Delete schedule
- `GET /api/schedule/delayed` - Get delayed start status
- `POST /api/schedule/delayed` - Set delayed start
- `DELETE /api/schedule/delayed` - Cancel delayed start

### Statistics
- `GET /api/statistics` - Get statistics summary
- `GET /api/statistics/history` - Get cycle history
- `DELETE /api/statistics/history` - Clear history

## Breaking Changes

### StateManager API Changes
- `setState()` now returns `bool` instead of `void`
- Must call `processCallbacks()` in main loop
- State queries are now thread-safe

### Component Access
- Use `ApplicationKernel::getInstance().get<Component>()` instead of global objects

## Migration Guide

1. **Update StateManager callbacks**
   ```cpp
   // Old
   stateManager.onStateChange([](WashState old, WashState new) { ... });
   
   // New - callbacks are async, must process in loop
   stateManager.onStateChange([](WashState old, WashState new) { ... });
   // In loop():
   stateManager.processCallbacks();
   ```

2. **Use ApplicationKernel**
   ```cpp
   // Old
   extern ConfigManager configManager;
   
   // New
   auto config = ApplicationKernel::getInstance().getConfigManager();
   ```

3. **Enable authentication** (recommended)
   ```cpp
   auto auth = ApplicationKernel::getInstance().getAuthMiddleware();
   auth->setEnabled(true);
   String token = auth->getAdminToken();
   // Save token securely!
   ```

## Dependencies Added

- `AsyncMqttClient@^0.9.0` - MQTT support

## File Structure

```
include/
├── ApplicationKernel.h    # NEW - DI container
├── AuthMiddleware.h       # NEW - Authentication
├── MQTTInterface.h        # NEW - MQTT integration
├── ScheduleManager.h      # NEW - Scheduling
├── StateManager.h         # UPDATED - Thread-safe
└── ... (existing)

src/
├── ApplicationKernel.cpp  # NEW
├── AuthMiddleware.cpp     # NEW
├── MQTTInterface.cpp      # NEW
├── ScheduleManager.cpp    # NEW
├── StateManager.cpp       # UPDATED
├── main.cpp               # UPDATED - Uses ApplicationKernel
└── ... (existing)

data/
├── config-mqtt.html       # NEW - MQTT config page
├── schedule.html          # NEW - Schedule management
├── statistics.html        # NEW - Statistics dashboard
├── setup-wizard.html      # NEW - Setup wizard
└── ... (existing)
```

## Known Issues

1. HTTPS/TLS not yet implemented (planned for v2.1)
2. Mobile app pending (planned for v2.1)
3. Voice assistant integration pending (planned for v2.1)

## Contributors

- Washka Team

## License

MIT License
