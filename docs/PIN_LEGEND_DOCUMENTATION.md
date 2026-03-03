# WROOM-32 Pin Legend Documentation

## Overview

The Pin Legend system provides comprehensive information about ESP32 WROOM-32 GPIO pins, including capabilities, restrictions, and recommendations for safe usage.

## Features

### 1. Comprehensive Pin Database

The system includes detailed information for all 40 GPIO pins (0-39) on the WROOM-32 module:

- **Pin Number**: GPIO number
- **Label**: Common label (e.g., D32, TX, RX)
- **Description**: Full pin description with alternate functions
- **Capabilities**: ADC1, ADC2, DAC, Touch, RTC, Digital I/O
- **Restrictions**: Strapping, Flash, Serial, Input Only, ADC2/WiFi conflict
- **Warnings**: Specific warnings about pin usage
- **Recommendation**: Whether the pin is recommended for general use

### 2. Pin Categories

#### Recommended Pins (Safe for General Use)
- GPIO 4, 5, 13, 14, 16-19, 21-23, 25-27, 32-33
- These pins can be safely used for most applications

#### Strapping Pins (Use with Caution)
- GPIO 0: Boot mode selection (must be HIGH during boot)
- GPIO 2: Boot mode selection (must be LOW during boot, often has onboard LED)
- GPIO 5: SDIO timing (generally safe but use with caution)
- GPIO 12: Flash voltage (boot fails if pulled HIGH)
- GPIO 15: Boot messages (boot fails if pulled HIGH)

#### Flash Pins (DO NOT USE)
- GPIO 6-11: Connected to SPI flash memory
- Using these pins will cause system instability

#### Serial Pins (Avoid if Serial Needed)
- GPIO 1: TX (Serial transmit)
- GPIO 3: RX (Serial receive)

#### Input Only Pins
- GPIO 34-39: Can only be used as inputs
- No internal pull-up or pull-down resistors available

### 3. Special Considerations

#### ADC2 and WiFi Conflict
Pins with ADC2 capability (GPIO 13, 14, 25-27) cannot be used for analog input when WiFi is active. This is a hardware limitation of the ESP32.

#### RTC Pins
Pins with RTC capability can be used in deep sleep mode for wake-up functionality.

#### Touch Pins
Some pins support capacitive touch sensing.

#### DAC Pins
GPIO 25 and 26 have digital-to-analog converters.

## API Endpoint

### GET /api/pins

Returns comprehensive pin legend data in JSON format.

**Response Format:**
```json
{
  "pins": [
    {
      "gpio": 32,
      "label": "D32",
      "description": "GPIO32/ADC1_4",
      "recommended": true,
      "warning": "",
      "capabilities": ["Digital I/O", "ADC1", "RTC", "Touch"],
      "restrictions": [],
      "canUseForOutput": true,
      "isInputOnly": false,
      "isStrapping": false,
      "isFlash": false
    },
    ...
  ]
}
```

## Web Interface Integration

The GPIO configuration page (`/config-gpio.html`) uses the pin legend to:

1. **Populate Pin Dropdowns**: All pin selects are populated with comprehensive pin information
2. **Color Coding**: 
   - Recommended pins: Normal text
   - Pins with warnings: Yellow text
   - Not recommended pins: Red text and disabled
3. **Help Text**: Dynamic help text shows warnings or capabilities for selected pins
4. **Pin Details Sidebar**: Detailed information about the currently selected pin
5. **Validation**: Prevents selection of invalid or dangerous pins

## Code Integration

### C++ Usage

```cpp
#include "PinLegend.h"

// Check if a pin is valid
bool valid = PinLegend::isPinValid(32);

// Check if a pin is recommended
bool recommended = PinLegend::isPinRecommended(32);

// Get pin warning
const char* warning = PinLegend::getPinWarning(12);

// Get pin description
String desc = PinLegend::getPinDescription(32);

// Get list of recommended pins
std::vector<uint8_t> pins = PinLegend::getRecommendedPins();

// Get full pin legend as JSON
String json = PinLegend::getPinLegendJson();

// Get detailed pin info
const PinInfo* info = PinLegend::getPinInfo(32);
if (info) {
    bool hasADC = info->hasCapability(CAP_ADC1);
    bool isStrapping = info->isStrapping();
    bool canOutput = info->canUseForOutput();
}
```

### JavaScript Usage

```javascript
// Load pin legend from API
fetch('/api/pins')
    .then(response => response.json())
    .then(data => {
        const pins = data.pins;
        
        // Find a specific pin
        const pin32 = pins.find(p => p.gpio === 32);
        
        // Check if recommended
        if (pin32.recommended) {
            console.log('Pin 32 is safe to use');
        }
        
        // Check for warnings
        if (pin32.warning) {
            console.warn('Warning:', pin32.warning);
        }
        
        // Check capabilities
        if (pin32.capabilities.includes('ADC1')) {
            console.log('Pin 32 has ADC1 capability');
        }
    });
```

## Validation in ConfigManager

The ConfigManager now uses PinLegend for validation:

```cpp
bool ConfigManager::isPinValid(uint8_t pin) {
    return PinLegend::isPinValid(pin);
}
```

This ensures that:
- Flash pins (6-11) are rejected
- Pins outside valid range (0-39) are rejected
- Pin information is consistent across the system

## Pin Map Reference

### Complete Pin List

| GPIO | Label | Description | Recommended | Notes |
|------|-------|-------------|-------------|-------|
| 0 | D0 | GPIO0/BOOT | ❌ | Strapping - boot mode |
| 1 | TX | GPIO1/U0TXD | ❌ | Serial TX |
| 2 | D2 | GPIO2/LED | ❌ | Strapping - boot mode, onboard LED |
| 3 | RX | GPIO3/U0RXD | ❌ | Serial RX |
| 4 | D4 | GPIO4/RTC | ✅ | Safe to use |
| 5 | D5 | GPIO5 | ✅ | Strapping - generally safe |
| 6-11 | - | Flash pins | ❌ | DO NOT USE |
| 12 | D12 | GPIO12/ADC2_5 | ❌ | Strapping - boot fails if HIGH |
| 13 | D13 | GPIO13/ADC2_4 | ✅ | ADC2 - WiFi conflict |
| 14 | D14 | GPIO14/ADC2_6 | ✅ | ADC2 - WiFi conflict |
| 15 | D15 | GPIO15/ADC2_3 | ❌ | Strapping - boot fails if HIGH |
| 16-19 | D16-D19 | GPIO16-19 | ✅ | Safe to use |
| 21-23 | D21-D23 | GPIO21-23 | ✅ | Safe to use (I2C) |
| 25 | D25 | GPIO25/ADC2_8/DAC1 | ✅ | ADC2/DAC - WiFi conflict |
| 26 | D26 | GPIO26/ADC2_9/DAC2 | ✅ | ADC2/DAC - WiFi conflict |
| 27 | D27 | GPIO27/ADC2_7 | ✅ | ADC2 - WiFi conflict |
| 32 | D32 | GPIO32/ADC1_4 | ✅ | Safe to use |
| 33 | D33 | GPIO33/ADC1_5 | ✅ | Safe to use |
| 34 | D34 | GPIO34/ADC1_6 | ✅ | Input only |
| 35 | D35 | GPIO35/ADC1_7 | ✅ | Input only |
| 36 | VP/D36 | GPIO36/ADC1_0/VP | ✅ | Input only |
| 39 | VN/D39 | GPIO39/ADC1_3/VN | ✅ | Input only |

## Best Practices

1. **Always use recommended pins** for critical functions
2. **Avoid strapping pins** unless you understand the boot implications
3. **Never use flash pins** (6-11)
4. **Remember ADC2/WiFi conflict** when using analog inputs
5. **Use input-only pins** (34-39) only for inputs
6. **Check pin warnings** before finalizing hardware design
7. **Test thoroughly** with actual hardware before deployment

## Requirements Validation

This implementation satisfies the following requirements:

- **Requirement 3.5**: Pin configuration with WROOM-32 legend
- **Requirement 9.1**: Display pin legend in configuration UI
- **Requirement 9.5**: Display warnings for invalid pin selections

The pin legend provides comprehensive information to help users make informed decisions about GPIO pin assignments and avoid common pitfalls with ESP32 hardware.
