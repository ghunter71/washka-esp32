# Washka ESP32 - Dishwasher Control System

ESP32-based intelligent dishwasher control system with web interface, REST API, and Telegram bot integration.

## Features

- 🌐 **Web Interface** - Bootstrap dark theme UI for configuration and control
- 🔌 **WiFi Manager** - Easy WiFi setup via captive portal
- 📡 **REST API** - Full programmatic control with OpenAPI documentation
- 💬 **Telegram Bot** - Remote control and notifications
- 🔄 **OTA Updates** - Wireless firmware updates
- 💾 **NVS Storage** - Persistent configuration storage
- 🔧 **GPIO Configuration** - Flexible pin mapping via web interface
- 🚰 **Smart Water Control** - Gerkon-based water level monitoring
- 🛡️ **Safety Features** - Actuator interlocks and emergency stop
- 📊 **Debug Logging** - Serial and WebSerial output

## Hardware Requirements

- ESP32 WROOM-32 module
- Relay modules for actuators (pump, valve, wash engine, powder dispenser)
- Gerkon (reed switch) for water level detection
- Push button for manual control
- Status LED

## Pin Configuration

Default pin assignments (configurable via web interface):

| Function | GPIO | Description |
|----------|------|-------------|
| Wash Engine | 5 | Main wash motor |
| Pump | 4 | Drain pump |
| Water Gerkon | 13 | Water level sensor |
| Powder Dispenser | 12 | Detergent dispenser |
| Water Valve | 14 | Water inlet valve |
| Button | 2 | Manual control button |
| LED | 15 | Status indicator |

## Software Setup

### Prerequisites

- [PlatformIO](https://platformio.org/) installed
- USB cable for initial programming

### Building and Flashing

1. Clone the repository
2. Open in PlatformIO
3. Build the project:
   ```bash
   pio run
   ```
4. Upload to ESP32:
   ```bash
   pio run --target upload
   ```
5. Upload filesystem (web files):
   ```bash
   pio run --target uploadfs
   ```

### Initial Configuration

1. On first boot, ESP32 creates WiFi AP: `Washka-Setup`
2. Connect to the AP (password: `washka123`)
3. Navigate to `http://192.168.4.1`
4. Configure WiFi credentials
5. Device reboots and connects to your network
6. Access web interface at device IP address

## Web Interface

Access the web interface at `http://<device-ip>/`

- **Dashboard** - System status and control
- **Configuration** - GPIO pins, timing, WiFi, Telegram
- **Manual Control** - Test individual actuators
- **Logs** - Real-time debug output via WebSerial

## REST API

API documentation available at `http://<device-ip>/api/docs`

### Example Endpoints

```bash
# Get system status
curl http://<device-ip>/api/status

# Start wash cycle
curl -X POST http://<device-ip>/api/control/start

# Get configuration
curl http://<device-ip>/api/config

# Update configuration
curl -X POST http://<device-ip>/api/config \
  -H "Content-Type: application/json" \
  -d '{"timing":{"washtime1":2400000}}'
```

## Telegram Bot

1. Create bot via [@BotFather](https://t.me/botfather)
2. Get bot token
3. Configure token in web interface
4. Send `/start` to bot to get your chat ID
5. Add chat ID to allowed list in web interface

### Bot Commands

- `/start` - Show available commands
- `/status` - Get current system status
- `/startwash` - Start wash cycle
- `/stop` - Stop current operation
- `/config` - Show current configuration

## OTA Updates

Upload new firmware via web interface at `http://<device-ip>/update`

## Development

### Project Structure

```
├── src/              # Source code
│   ├── main.cpp      # Main entry point
│   └── components/   # Component implementations
├── include/          # Header files
├── data/             # Web interface files (SPIFFS)
├── test/             # Unit and property tests
├── platformio.ini    # PlatformIO configuration
└── partitions.csv    # ESP32 partition table
```

### Running Tests

```bash
pio test
```

## Safety Features

- Mutual exclusion prevents simultaneous fill/drain
- Watchdog timer (120s timeout)
- Emergency stop functionality
- Sensor validation
- Timeout protection on all operations

## Troubleshooting

### Cannot connect to WiFi AP

- Check that AP is enabled (first boot or after factory reset)
- Try password: `washka123`
- Reset device and try again

### Web interface not loading

- Check device IP address in serial monitor
- Ensure device is connected to WiFi
- Try accessing via `http://<ip>/` (not https)

### OTA update fails

- Ensure system is in IDLE state
- Check available flash space
- Verify firmware file is valid

## License

MIT License - see LICENSE file for details

## Support

For issues and questions, please open an issue on GitHub.
