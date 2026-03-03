# Quick Start Guide

## 1. Hardware Setup

Connect your ESP32 WROOM-32 module:

| Component | GPIO | Notes |
|-----------|------|-------|
| Wash Engine Relay | 5 | Main wash motor |
| Pump Relay | 4 | Drain pump |
| Water Gerkon | 13 | Reed switch (pull-up) |
| Powder Dispenser | 12 | Detergent dispenser |
| Water Valve | 14 | Water inlet valve |
| Button | 2 | Manual control (pull-up) |
| Status LED | 15 | Active HIGH |

## 2. Software Setup

### Install PlatformIO

```bash
# Using pip
pip install platformio

# Or download PlatformIO IDE
# https://platformio.org/platformio-ide
```

### Clone and Build

```bash
# Clone repository
git clone https://github.com/YOUR_USERNAME/washka-esp32.git
cd washka-esp32

# Build firmware
pio run

# Build filesystem
pio run --target buildfs
```

## 3. Flash ESP32

### Option A: USB Flash (First Time)

```bash
# Flash everything (firmware + filesystem)
pio run --target upload
pio run --target uploadfs
```

### Option B: Manual Flash with esptool

```bash
esptool.py --port COM5 --baud 921600 \
  --before default_reset --after hard_reset write_flash \
  0x1000 .pio/build/esp32dev/bootloader.bin \
  0x8000 .pio/build/esp32dev/partitions.bin \
  0xe000 .pio/build/esp32dev/boot_app0.bin \
  0x10000 .pio/build/esp32dev/firmware.bin \
  0x310000 .pio/build/esp32dev/littlefs.bin
```

## 4. Initial Configuration

1. **Connect to WiFi AP**
   - SSID: `Washka-Setup`
   - Password: `washka123`

2. **Open Web Interface**
   - Navigate to: `http://192.168.4.1`

3. **Configure WiFi**
   - Enter your WiFi credentials
   - Device will reboot and connect

4. **Access Device**
   - Find device IP in your router
   - Or check serial monitor output
   - Open: `http://<device-ip>/`

## 5. Configure Telegram Bot (Optional)

1. Create bot via [@BotFather](https://t.me/botfather)
2. Get bot token
3. Open web interface → Configuration → Telegram
4. Enter bot token
5. Send `/start` to bot
6. Copy your chat ID
7. Add chat ID to allowed list

## 6. Test System

### Via Web Interface

1. Open `http://<device-ip>/`
2. Go to Manual Control
3. Test each actuator individually
4. Verify gerkon sensor reading

### Via REST API

```bash
# Get status
curl http://<device-ip>/api/status

# Start wash cycle
curl -X POST http://<device-ip>/api/control/start

# Stop operation
curl -X POST http://<device-ip>/api/control/stop
```

### Via Telegram

```
/status - Check system status
/startwash - Start wash cycle
/stop - Emergency stop
```

## 7. OTA Updates

After initial flash, update wirelessly:

```bash
# Update firmware
curl -F "update=@.pio/build/esp32dev/firmware.bin" \
  http://<device-ip>/update

# Update filesystem
curl -F "filesystem=@.pio/build/esp32dev/littlefs.bin" \
  http://<device-ip>/api/update/filesystem
```

## Troubleshooting

### Device not responding
- Check power supply (5V, min 500mA)
- Verify USB cable supports data transfer
- Press EN button to reset

### Cannot connect to WiFi AP
- Wait 30 seconds after boot
- Check WiFi is enabled on your device
- Try factory reset (hold button during boot)

### Web interface not loading
- Check device IP address
- Ensure device connected to WiFi
- Try clearing browser cache

### Actuators not working
- Verify relay connections
- Check GPIO configuration in web interface
- Test with multimeter

## Next Steps

- Read full documentation in `docs/`
- Configure timing parameters
- Set up safety interlocks
- Customize wash cycles
- Enable debug logging

## Support

- Documentation: `docs/`
- API Reference: `http://<device-ip>/api/docs`
- Issues: GitHub Issues
- Tests: `pio test`

Happy washing! 🚿
