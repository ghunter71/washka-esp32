# Requirements Document

## Introduction

Данный документ описывает требования к портированию и модернизации системы управления посудомоечной машиной с ESP8266 на ESP32. Система должна обеспечивать автоматическое управление циклами мойки с возможностью настройки через веб-интерфейс, управления через API и Telegram-бот, а также поддержку OTA-обновлений.

## Glossary

- **WashkaSystem**: Система управления посудомоечной машиной на базе ESP32
- **WebInterface**: Веб-интерфейс на базе Bootstrap с темной темой для настройки и управления
- **WifiManager**: Компонент для настройки WiFi-подключения
- **PinMapper**: Компонент для настройки используемых GPIO-пинов
- **GerkonSensor**: Геркон (магнитный датчик) для контроля уровня воды
- **WaterControl**: Механизм контроля набора воды по количеству срабатываний геркона
- **OTAUpdater**: Компонент для беспроводного обновления прошивки
- **TelegramBot**: Бот для управления системой через Telegram
- **RestAPI**: RESTful API для программного управления системой
- **WebSerial**: Веб-интерфейс для просмотра отладочной информации
- **StateManager**: Компонент для управления состоянием машины и восстановления после сбоев
- **WROOM-32**: Модуль ESP32 с определенной распиновкой

## Requirements

### Requirement 1

**User Story:** Как пользователь, я хочу портировать систему с ESP8266 на ESP32, чтобы использовать более мощную платформу с большими возможностями.

#### Acceptance Criteria

1. WHEN the system starts THEN the WashkaSystem SHALL initialize on ESP32 hardware using ESP32-specific libraries
2. WHEN GPIO pins are accessed THEN the WashkaSystem SHALL use ESP32-compatible pin definitions
3. WHEN WiFi connection is established THEN the WashkaSystem SHALL use ESP32 WiFi library instead of ESP8266WiFi
4. WHEN the system operates THEN the WashkaSystem SHALL maintain all existing functionality from ESP8266 version
5. WHEN async operations are performed THEN the WashkaSystem SHALL use AsyncTCP library compatible with ESP32

### Requirement 2

**User Story:** Как пользователь, я хочу настраивать WiFi-подключение через веб-интерфейс, чтобы не перепрошивать устройство при смене сети.

#### Acceptance Criteria

1. WHEN the system cannot connect to saved WiFi THEN the WashkaSystem SHALL start in Access Point mode with captive portal
2. WHEN a user connects to the Access Point THEN the WashkaSystem SHALL display WiFi configuration page
3. WHEN WiFi credentials are submitted THEN the WashkaSystem SHALL validate and save them to persistent storage
4. WHEN valid WiFi credentials are saved THEN the WashkaSystem SHALL attempt connection to the specified network
5. WHEN WiFi connection succeeds THEN the WashkaSystem SHALL switch from AP mode to station mode

### Requirement 3

**User Story:** Как пользователь, я хочу настраивать используемые GPIO-пины через веб-интерфейс, чтобы адаптировать систему под разные аппаратные конфигурации.

#### Acceptance Criteria

1. WHEN the configuration page is displayed THEN the WebInterface SHALL show all configurable GPIO pins with current assignments
2. WHEN GPIO pin assignments are changed THEN the WashkaSystem SHALL validate that pins are not duplicated
3. WHEN GPIO configuration is saved THEN the WashkaSystem SHALL store pin assignments to persistent storage
4. WHEN the system restarts THEN the WashkaSystem SHALL load GPIO configuration from persistent storage
5. WHEN displaying pin options THEN the WebInterface SHALL show WROOM-32 pin legend (e.g., "ADC1_4/GPIO32/pin12 - D32")

### Requirement 4

**User Story:** Как пользователь, я хочу настраивать временные интервалы работы через веб-интерфейс, чтобы оптимизировать циклы мойки.

#### Acceptance Criteria

1. WHEN the configuration page is displayed THEN the WebInterface SHALL show all timing parameters with current values in human-readable format
2. WHEN timing values are modified THEN the WashkaSystem SHALL validate that values are within acceptable ranges
3. WHEN timing configuration is saved THEN the WashkaSystem SHALL store values to persistent storage
4. WHEN the system restarts THEN the WashkaSystem SHALL load timing configuration from persistent storage
5. WHEN invalid timing values are submitted THEN the WashkaSystem SHALL reject the configuration and display error message

### Requirement 5

**User Story:** Как пользователь, я хочу использовать веб-интерфейс на темной теме Bootstrap, чтобы иметь современный и удобный интерфейс управления.

#### Acceptance Criteria

1. WHEN the web interface loads THEN the WebInterface SHALL display using Bootstrap framework with dark theme
2. WHEN the interface is viewed on mobile devices THEN the WebInterface SHALL be responsive and usable
3. WHEN configuration forms are displayed THEN the WebInterface SHALL use Bootstrap form components with validation
4. WHEN actions are performed THEN the WebInterface SHALL provide visual feedback using Bootstrap alerts and modals
5. WHEN the interface is accessed THEN the WebInterface SHALL load quickly without external CDN dependencies

### Requirement 6

**User Story:** Как пользователь, я хочу управлять системой через веб-интерфейс, чтобы запускать, останавливать и контролировать процесс мойки.

#### Acceptance Criteria

1. WHEN the control page is displayed THEN the WebInterface SHALL show current system state and available actions
2. WHEN a start command is issued THEN the WashkaSystem SHALL begin the wash cycle from the appropriate state
3. WHEN a stop command is issued THEN the WashkaSystem SHALL safely halt operations and save current state
4. WHEN the system is running THEN the WebInterface SHALL display real-time status updates
5. WHEN manual control is requested THEN the WebInterface SHALL allow individual actuator control for testing

### Requirement 7

**User Story:** Как разработчик, я хочу использовать REST API для программного управления системой, чтобы интегрировать её с другими системами.

#### Acceptance Criteria

1. WHEN API documentation is requested THEN the WashkaSystem SHALL serve OpenAPI/Swagger documentation at /api/docs
2. WHEN API endpoints are called THEN the RestAPI SHALL return responses in JSON format with appropriate HTTP status codes
3. WHEN authentication is required THEN the RestAPI SHALL validate API keys or tokens before processing requests
4. WHEN state queries are made THEN the RestAPI SHALL return current system state including all sensor readings
5. WHEN control commands are sent THEN the RestAPI SHALL validate parameters and execute commands safely

### Requirement 8

**User Story:** Как пользователь, я хочу управлять системой через Telegram-бот, чтобы контролировать мойку удаленно с мобильного устройства.

#### Acceptance Criteria

1. WHEN Telegram bot token is configured THEN the TelegramBot SHALL establish connection to Telegram API
2. WHEN a user sends /start command THEN the TelegramBot SHALL respond with available commands and current status
3. WHEN control commands are received THEN the TelegramBot SHALL validate user authorization before execution
4. WHEN system state changes THEN the TelegramBot SHALL send notifications to subscribed users
5. WHEN status is requested THEN the TelegramBot SHALL respond with current state, progress, and sensor readings

### Requirement 9

**User Story:** Как пользователь, я хочу видеть подробную информацию о распиновке WROOM-32, чтобы правильно подключать компоненты.

#### Acceptance Criteria

1. WHEN pin configuration page is displayed THEN the WebInterface SHALL show pin legend with format "ADC1_4/GPIO32/pin12 - D32"
2. WHEN hovering over pin fields THEN the WebInterface SHALL display tooltip with pin capabilities and restrictions
3. WHEN selecting pins THEN the WebInterface SHALL highlight pins that cannot be used (strapping pins, flash pins)
4. WHEN documentation is accessed THEN the WebInterface SHALL provide link to full WROOM-32 pinout diagram
5. WHEN invalid pins are selected THEN the WebInterface SHALL display warning about pin limitations

### Requirement 10

**User Story:** Как пользователь, я хочу улучшенный механизм контроля набора воды по срабатываниям геркона, чтобы точно контролировать уровень воды.

#### Acceptance Criteria

1. WHEN water filling starts THEN the WaterControl SHALL initialize gerkon counter and start timeout timer
2. WHEN gerkon state changes from LOW to HIGH THEN the WaterControl SHALL increment counter
3. WHEN gerkon counter reaches configured threshold THEN the WaterControl SHALL close water valve
4. WHEN timeout expires before threshold THEN the WaterControl SHALL close water valve and log warning
5. WHEN gerkon readings are noisy THEN the WaterControl SHALL implement debouncing to prevent false counts

### Requirement 11

**User Story:** Как пользователь, я хочу поддержку OTA-обновлений, чтобы обновлять прошивку без физического доступа к устройству.

#### Acceptance Criteria

1. WHEN OTA update is initiated THEN the OTAUpdater SHALL verify firmware signature before applying
2. WHEN update is in progress THEN the OTAUpdater SHALL display progress percentage
3. WHEN update completes successfully THEN the OTAUpdater SHALL reboot system with new firmware
4. WHEN update fails THEN the OTAUpdater SHALL rollback to previous firmware version
5. WHEN system is performing critical operations THEN the OTAUpdater SHALL prevent update initiation

### Requirement 12

**User Story:** Как разработчик, я хочу подробную отладочную информацию в Serial и WebSerial, чтобы диагностировать проблемы.

#### Acceptance Criteria

1. WHEN system operates THEN the WashkaSystem SHALL output timestamped log messages to Serial port
2. WHEN WebSerial is accessed THEN the WashkaSystem SHALL display real-time log stream in web browser
3. WHEN DEBUG command is sent to WebSerial THEN the WashkaSystem SHALL output detailed system state dump
4. WHEN errors occur THEN the WashkaSystem SHALL log error details with stack trace when available
5. WHEN debug mode is enabled THEN the WashkaSystem SHALL include verbose sensor readings and timing information

### Requirement 13

**User Story:** Как пользователь, я хочу сохранение конфигурации в энергонезависимой памяти, чтобы настройки сохранялись после перезагрузки.

#### Acceptance Criteria

1. WHEN configuration is changed THEN the WashkaSystem SHALL save settings to NVS (Non-Volatile Storage)
2. WHEN system boots THEN the WashkaSystem SHALL load configuration from NVS with default fallback values
3. WHEN NVS is corrupted THEN the WashkaSystem SHALL detect corruption and restore factory defaults
4. WHEN factory reset is requested THEN the WashkaSystem SHALL clear all NVS data and restart
5. WHEN configuration is exported THEN the WashkaSystem SHALL generate JSON file with all settings


### Requirement 14

**User Story:** Как пользователь, я хочу чтобы система всегда запускалась в режиме ожидания, чтобы иметь предсказуемое поведение после включения питания.

#### Acceptance Criteria

1. WHEN system boots THEN the StateManager SHALL initialize in IDLE state regardless of previous activity
2. WHEN power loss occurs during operation THEN the StateManager SHALL start in IDLE state after power restoration
3. WHEN system starts THEN the StateManager SHALL log startup event with timestamp
4. WHEN IDLE state is active THEN the WashkaSystem SHALL wait for user command to start wash cycle
5. WHEN system is in IDLE state THEN the WashkaSystem SHALL allow configuration changes


### Requirement 15

**User Story:** Как пользователь, я хочу безопасное управление актуаторами, чтобы предотвратить повреждение оборудования.

#### Acceptance Criteria

1. WHEN multiple actuators are controlled THEN the WashkaSystem SHALL enforce mutual exclusion rules (e.g., no simultaneous fill and drain)
2. WHEN timeout occurs during operation THEN the WashkaSystem SHALL automatically disable all actuators
3. WHEN emergency stop is triggered THEN the WashkaSystem SHALL immediately disable all outputs and enter safe state
4. WHEN sensor readings are invalid THEN the WashkaSystem SHALL halt operations and trigger alarm
5. WHEN watchdog timer expires THEN the WashkaSystem SHALL reset and log watchdog event

### Requirement 16

**User Story:** Как пользователь, я хочу надежное определение нажатия кнопки, чтобы система корректно реагировала на физические нажатия.

#### Acceptance Criteria

1. WHEN button pin state transitions from HIGH to LOW to HIGH THEN the WashkaSystem SHALL register one button press event
2. WHEN button state changes occur within debounce period THEN the WashkaSystem SHALL ignore intermediate transitions
3. WHEN button press is detected THEN the WashkaSystem SHALL execute configured action (start cycle, stop cycle, or toggle state)
4. WHEN button is held for extended period THEN the WashkaSystem SHALL distinguish between short press and long press actions
5. WHEN button readings are noisy THEN the WashkaSystem SHALL implement debouncing to prevent false press detection