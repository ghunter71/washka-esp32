# Implementation Plan

- [x] 1. Setup ESP32 project structure and dependencies


  - Create PlatformIO project for ESP32
  - Configure platformio.ini with ESP32 board and required libraries
  - Setup partition table for OTA support
  - Create directory structure: src/, include/, data/ (for web files)
  - _Requirements: 1.1, 1.3, 1.5_

- [x] 2. Implement ConfigManager for NVS storage



  - Create ConfigManager class with NVS read/write operations
  - Implement WiFi credentials storage and retrieval
  - Implement GPIO pin configuration storage
  - Implement timing configuration storage
  - Implement gerkon and Telegram configuration storage
  - Add factory reset functionality
  - Add JSON export/import functionality
  - _Requirements: 2.3, 3.3, 3.4, 4.3, 4.4, 13.1, 13.2, 13.4, 13.5_

- [x] 2.1 Write property test for GPIO pin validation


  - **Property 1: ESP32 pin validation**
  - **Validates: Requirements 1.2, 3.2**

- [x] 2.2 Write property test for WiFi credentials persistence

  - **Property 2: WiFi credentials persistence round-trip**
  - **Validates: Requirements 2.3**

- [x] 2.3 Write property test for GPIO configuration persistence

  - **Property 3: GPIO configuration persistence round-trip**
  - **Validates: Requirements 3.3, 3.4**

- [x] 2.4 Write property test for timing validation

  - **Property 4: Timing configuration validation**
  - **Validates: Requirements 4.2, 4.5**

- [x] 2.5 Write property test for timing persistence

  - **Property 5: Timing configuration persistence round-trip**
  - **Validates: Requirements 4.3, 4.4**

- [x] 2.6 Write property test for configuration export/import

  - **Property 22: Configuration export/import round-trip**
  - **Validates: Requirements 13.5**

- [x] 2.7 Write property test for NVS persistence

  - **Property 21: Configuration NVS persistence**
  - **Validates: Requirements 13.1**

- [x] 3. Implement StateManager for wash cycle control







  - Create StateManager class with state machine logic
  - Implement state transitions for wash cycle phases
  - Add start/stop/pause/resume cycle methods
  - Implement progress tracking and time estimation
  - Ensure system always starts in IDLE state
  - Add state change callbacks
  - _Requirements: 6.2, 6.3, 14.1, 14.2, 14.3, 14.4_

- [x] 3.1 Write property test for state transition safety


  - **Property 6: State transition safety**
  - **Validates: Requirements 6.2**

- [x] 3.2 Write property test for stop command safety


  - **Property 7: Stop command safety**
  - **Validates: Requirements 6.3**

- [x] 4. Implement ActuatorManager for hardware control


  - Create ActuatorManager class with GPIO control
  - Implement individual actuator control methods
  - Add safety checks for mutual exclusion (no simultaneous fill/drain)
  - Implement emergency stop functionality
  - Add actuator status reporting
  - _Requirements: 15.1, 15.2, 15.3_

- [x] 4.1 Write property test for actuator mutual exclusion


  - **Property 23: Actuator mutual exclusion**
  - **Validates: Requirements 15.1**

- [x] 4.2 Write property test for timeout safety


  - **Property 24: Timeout safety shutdown**
  - **Validates: Requirements 15.2**

- [x] 4.3 Write property test for emergency stop


  - **Property 25: Emergency stop completeness**
  - **Validates: Requirements 15.3**

- [x] 5. Implement WaterControl for gerkon-based water level



  - Create WaterControl class with gerkon monitoring
  - Implement interrupt-based gerkon counting with debouncing
  - Add water fill control with threshold and timeout
  - Implement gerkon trigger callbacks
  - _Requirements: 10.1, 10.2, 10.3, 10.4, 10.5_

- [x] 5.1 Write property test for gerkon counter increment


  - **Property 14: Gerkon counter increment**
  - **Validates: Requirements 10.2**

- [x] 5.2 Write property test for gerkon threshold trigger


  - **Property 15: Gerkon threshold trigger**
  - **Validates: Requirements 10.3**

- [x] 5.3 Write property test for gerkon debouncing


  - **Property 16: Gerkon debouncing**
  - **Validates: Requirements 10.5**

- [x] 5.4 Write property test for invalid sensor halt


  - **Property 26: Invalid sensor halt**
  - **Validates: Requirements 15.4**

- [ ] 5a. Implement ButtonControl for button press detection

  - Create ButtonControl class with state machine for HIGH → LOW → HIGH detection
  - Implement debouncing to filter noise
  - Add long press detection with configurable threshold
  - Implement button press callbacks
  - Add button state query methods
  - _Requirements: 16.1, 16.2, 16.3, 16.4, 16.5_

- [ ] 5a.1 Write property test for button press sequence detection

  - **Property 27: Button press detection sequence**
  - **Validates: Requirements 16.1**

- [ ] 5a.2 Write property test for button debounce filtering

  - **Property 28: Button debounce filtering**
  - **Validates: Requirements 16.2, 16.5**

- [ ] 5a.3 Write property test for button action execution

  - **Property 29: Button action execution**
  - **Validates: Requirements 16.3**

- [ ] 5a.4 Write property test for button long press distinction

  - **Property 30: Button long press distinction**
  - **Validates: Requirements 16.4**

- [x] 6. Implement WiFiManager with AP mode and captive portal


  - Create WiFiManager class for WiFi connection management
  - Implement connection attempt with saved credentials
  - Add Access Point mode with captive portal on connection failure
  - Create WiFi configuration web page
  - Implement credential validation and saving
  - _Requirements: 2.1, 2.2, 2.4, 2.5_

- [x] 7. Create web interface HTML/CSS/JS with Bootstrap dark theme





  - Embed Bootstrap 5.3 CSS and JS (minified, gzipped)
  - Create main control page with system status display
  - Create configuration page for GPIO pins with WROOM-32 legend
  - Create configuration page for timing parameters
  - Create configuration page for WiFi and Telegram settings
  - Add manual actuator control for testing
  - Implement WebSocket for real-time status updates
  - _Requirements: 5.1, 5.3, 5.5, 6.1, 6.4, 9.1, 9.4_


- [x] 8. Implement WebInterface component




  - Create WebInterface class to serve web pages
  - Implement route handlers for all pages
  - Add WebSocket handler for real-time updates
  - Implement form submission handlers
  - Add static resource serving (Bootstrap, CSS, JS)
  - _Requirements: 3.1, 4.1, 5.2, 6.1, 6.5_


- [x] 9. Implement RestAPI with JSON endpoints




  - Create RestAPI class with endpoint handlers
  - Implement GET /api/status endpoint
  - Implement GET /api/config endpoint
  - Implement POST /api/config endpoint with validation
  - Implement POST /api/control/start endpoint
  - Implement POST /api/control/stop endpoint
  - Implement POST /api/control/pause endpoint
  - Create OpenAPI/Swagger documentation at /api/docs
  - Add API authentication with token validation
  - _Requirements: 7.1, 7.2, 7.3, 7.4, 7.5_

- [x] 9.1 Write property test for API JSON response validity







  - **Property 8: API JSON response validity**
  - **Validates: Requirements 7.2**


- [x] 9.2 Write property test for API authentication






  - **Property 9: API authentication enforcement**
  - **Validates: Requirements 7.3**

- [x] 9.3 Write property test for API parameter validation







  - **Property 10: API parameter validation**
  - **Validates: Requirements 7.5**





- [ ] 10. Implement TelegramInterface for bot control

  - Create TelegramInterface class with UniversalTelegramBot
  - Implement bot initialization with token from config
  - Add command handlers: /start, /status, /startwash, /stop, /config



  - Implement chat ID authorization check

  - Add state change notifications







  - Add error and completion notifications
  - _Requirements: 8.1, 8.2, 8.3, 8.4, 8.5_

- [x] 10.1 Write property test for Telegram authorization




  - **Property 11: Telegram authorization enforcement**
  - **Validates: Requirements 8.3**


- [ ] 10.2 Write property test for Telegram notifications
  - **Property 12: Telegram state change notifications**
  - **Validates: Requirements 8.4**

- [x] 11. Implement DebugLogger for Serial and WebSerial



  - Create DebugLogger class with log level support
  - Implement timestamped logging to Serial
  - Integrate WebSerial for browser-based log viewing
  - Add DEBUG command handler for system state dump
  - Implement log level filtering
  - _Requirements: 12.1, 12.2, 12.3, 12.4, 12.5_

- [x] 11.1 Write property test for log timestamps






  - **Property 19: Log timestamp presence**
  - **Validates: Requirements 12.1**


- [x] 11.2 Write property test for error logging






  - **Property 20: Error logging completeness**
  - **Validates: Requirements 12.4**


- [x] 12. Implement OTAUpdater with AsyncElegantOTA




  - Create OTAUpdater class with AsyncElegantOTA integration
  - Add safety check to prevent updates during critical operations
  - Implement progress callbacks
  - Add firmware signature verification (if applicable)
  - _Requirements: 11.1, 11.2, 11.3, 11.5_

- [x] 12.1 Write property test for OTA critical state blocking







  - **Property 18: OTA critical state blocking**
  - **Validates: Requirements 11.5**

- [x] 13. Integrate all components in main.cpp



  - Initialize all managers and components in setup()
  - Wire StateManager callbacks to TelegramInterface and WebInterface
  - Connect WaterControl callbacks to DebugLogger
  - Connect ButtonControl callbacks to StateManager for cycle control
  - Implement main loop with watchdog feeding
  - Add error handling and recovery logic
  - _Requirements: 1.4, 15.5, 16.3_


- [x] 14. Create WROOM-32 pin legend data and UI




  - Define WROOM-32 pin map with labels and descriptions
  - Add pin capability information (ADC, strapping, etc.)
  - Implement pin validation with warnings for restricted pins
  - Display pin legend in configuration UI
  - _Requirements: 3.5, 9.1, 9.5_

