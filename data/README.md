# Data Directory

This directory contains static web files that will be uploaded to SPIFFS/LittleFS on the ESP32.

## Contents

### HTML Pages
- `index.html` - Main control page with system status and wash cycle control
- `config-gpio.html` - GPIO pin configuration with WROOM-32 legend
- `config-timing.html` - Timing parameters configuration
- `config-system.html` - WiFi, Telegram, and system settings

### CSS Stylesheets
- `bootstrap.min.css` - Bootstrap 5.3 framework (see BOOTSTRAP_SETUP.md)
- `style.css` - Custom dark theme styles

### JavaScript Files
- `bootstrap.bundle.min.js` - Bootstrap 5.3 with Popper (see BOOTSTRAP_SETUP.md)
- `app.js` - Main control page logic with WebSocket
- `config-gpio.js` - GPIO configuration logic
- `config-timing.js` - Timing configuration logic
- `config-system.js` - System configuration logic

## Setup

1. Follow instructions in `BOOTSTRAP_SETUP.md` to download Bootstrap files
2. Upload all files to ESP32 SPIFFS/LittleFS using PlatformIO:
   ```bash
   pio run --target uploadfs
   ```

## Features

- **Dark Theme**: Bootstrap 5.3 dark theme with custom enhancements
- **Responsive Design**: Mobile-friendly interface
- **Real-time Updates**: WebSocket connection for live status
- **Manual Control**: Individual actuator testing
- **Configuration Management**: Export/import, factory reset
- **Pin Legend**: WROOM-32 specific pin information and warnings

## File Size

Total size (excluding Bootstrap): ~50 KB
With Bootstrap: ~240 KB (uncompressed) or ~40 KB (gzipped)
