# ESP32 WROOM-32 Pin Reference Guide

## Quick Reference Table

| GPIO | Label | Type | Capabilities | Restrictions | Recommended | Notes |
|------|-------|------|--------------|--------------|-------------|-------|
| 0 | D0 | I/O | RTC, Touch | Strapping | ❌ | Boot mode - must be HIGH |
| 1 | TX | I/O | Serial | Serial | ❌ | Serial TX |
| 2 | D2 | I/O | RTC, Touch | Strapping | ❌ | Boot mode - must be LOW, onboard LED |
| 3 | RX | I/O | Serial | Serial | ❌ | Serial RX |
| 4 | D4 | I/O | RTC, Touch | None | ✅ | Safe to use |
| 5 | D5 | I/O | - | Strapping | ✅ | SDIO timing - generally safe |
| 6 | SCK | - | - | Flash | ❌ | DO NOT USE - Flash SCK |
| 7 | SD0 | - | - | Flash | ❌ | DO NOT USE - Flash D0 |
| 8 | SD1 | - | - | Flash | ❌ | DO NOT USE - Flash D1 |
| 9 | SD2 | - | - | Flash | ❌ | DO NOT USE - Flash D2 |
| 10 | SD3 | - | - | Flash | ❌ | DO NOT USE - Flash D3 |
| 11 | CMD | - | - | Flash | ❌ | DO NOT USE - Flash CMD |
| 12 | D12 | I/O | ADC2_5, RTC, Touch | Strapping | ❌ | Boot fails if HIGH |
| 13 | D13 | I/O | ADC2_4, RTC, Touch | ADC2/WiFi | ✅ | ADC2 unavailable with WiFi |
| 14 | D14 | I/O | ADC2_6, RTC, Touch | ADC2/WiFi | ✅ | ADC2 unavailable with WiFi |
| 15 | D15 | I/O | ADC2_3, RTC, Touch | Strapping | ❌ | Boot fails if HIGH |
| 16 | D16 | I/O | - | None | ✅ | Safe to use |
| 17 | D17 | I/O | - | None | ✅ | Safe to use |
| 18 | D18 | I/O | - | None | ✅ | Safe to use |
| 19 | D19 | I/O | - | None | ✅ | Safe to use |
| 21 | D21 | I/O | - | None | ✅ | Safe to use (I2C SDA) |
| 22 | D22 | I/O | - | None | ✅ | Safe to use (I2C SCL) |
| 23 | D23 | I/O | - | None | ✅ | Safe to use |
| 25 | D25 | I/O | ADC2_8, DAC1, RTC | ADC2/WiFi | ✅ | DAC output, ADC2 unavailable with WiFi |
| 26 | D26 | I/O | ADC2_9, DAC2, RTC | ADC2/WiFi | ✅ | DAC output, ADC2 unavailable with WiFi |
| 27 | D27 | I/O | ADC2_7, RTC, Touch | ADC2/WiFi | ✅ | ADC2 unavailable with WiFi |
| 32 | D32 | I/O | ADC1_4, RTC, Touch | None | ✅ | Safe to use |
| 33 | D33 | I/O | ADC1_5, RTC, Touch | None | ✅ | Safe to use |
| 34 | D34 | Input | ADC1_6, RTC | Input Only | ✅ | Input only - no pull-up/down |
| 35 | D35 | Input | ADC1_7, RTC | Input Only | ✅ | Input only - no pull-up/down |
| 36 | VP/D36 | Input | ADC1_0, RTC | Input Only | ✅ | Input only - no pull-up/down |
| 39 | VN/D39 | Input | ADC1_3, RTC | Input Only | ✅ | Input only - no pull-up/down |

## Pin Categories

### ✅ Recommended for General Use (Output)
```
GPIO: 4, 5, 13, 14, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33
```
These pins are safe for most applications and can be used for digital output.

### ✅ Recommended for Input Only
```
GPIO: 34, 35, 36, 39
```
These pins can only be used as inputs. No internal pull-up or pull-down resistors.

### ⚠️ Use with Caution (Strapping Pins)
```
GPIO: 0, 2, 5, 12, 15
```
These pins affect boot behavior. Incorrect states during boot can prevent the ESP32 from starting.

### ❌ Do Not Use
```
GPIO: 1, 3 (Serial - if Serial needed)
GPIO: 6, 7, 8, 9, 10, 11 (Flash - never use)
```

## Strapping Pin Details

### GPIO 0 (Boot Mode)
- **Function**: Determines boot mode
- **Boot Requirement**: Must be HIGH for normal boot
- **Usage**: Can be used after boot, but requires pull-up resistor
- **Risk**: If LOW during boot, enters download mode

### GPIO 2 (Boot Mode)
- **Function**: Boot mode selection
- **Boot Requirement**: Must be LOW for normal boot (floating is OK)
- **Usage**: Often connected to onboard LED
- **Risk**: If HIGH during boot, may fail to boot

### GPIO 5 (SDIO Timing)
- **Function**: SDIO timing in SDIO slave mode
- **Boot Requirement**: No strict requirement
- **Usage**: Generally safe to use
- **Risk**: Low risk, but avoid if using SDIO

### GPIO 12 (Flash Voltage)
- **Function**: Selects flash voltage (1.8V vs 3.3V)
- **Boot Requirement**: Must be LOW for 3.3V flash (most common)
- **Usage**: Avoid using or ensure LOW during boot
- **Risk**: HIGH during boot = boot failure

### GPIO 15 (Boot Messages)
- **Function**: Controls boot message output
- **Boot Requirement**: Must be LOW for normal boot
- **Usage**: Avoid using or ensure LOW during boot
- **Risk**: HIGH during boot = boot failure

## Special Function Pins

### ADC (Analog to Digital Converter)

#### ADC1 Channels (Always Available)
```
GPIO 32 - ADC1_4
GPIO 33 - ADC1_5
GPIO 34 - ADC1_6
GPIO 35 - ADC1_7
GPIO 36 - ADC1_0
GPIO 39 - ADC1_3
```

#### ADC2 Channels (Unavailable when WiFi Active)
```
GPIO 13 - ADC2_4
GPIO 14 - ADC2_6
GPIO 25 - ADC2_8
GPIO 26 - ADC2_9
GPIO 27 - ADC2_7
```
**Important**: ADC2 cannot be used when WiFi is active. Use ADC1 for analog inputs in WiFi applications.

### DAC (Digital to Analog Converter)
```
GPIO 25 - DAC1
GPIO 26 - DAC2
```
8-bit DAC for analog output (0-3.3V).

### Touch Sensors
```
GPIO 0, 2, 4, 12, 13, 14, 15, 27, 32, 33
```
Capacitive touch sensing capability.

### RTC GPIO
```
GPIO 0, 2, 4, 12, 13, 14, 15, 25, 26, 27, 32, 33, 34, 35, 36, 39
```
Can be used in deep sleep mode for wake-up.

### I2C (Default Pins)
```
GPIO 21 - SDA (Data)
GPIO 22 - SCL (Clock)
```
Can be reassigned to other pins if needed.

### SPI (Default Pins)
```
GPIO 18 - SCK (Clock)
GPIO 19 - MISO (Master In Slave Out)
GPIO 23 - MOSI (Master Out Slave In)
GPIO 5  - CS (Chip Select)
```
Can be reassigned to other pins if needed.

## Common Use Cases

### Digital Output (LED, Relay, etc.)
**Recommended Pins**: 4, 16, 17, 18, 19, 21, 22, 23, 32, 33

### Digital Input (Button, Switch, etc.)
**Recommended Pins**: 4, 16, 17, 18, 19, 21, 22, 23, 32, 33, 34, 35, 36, 39

### Analog Input (Sensor, Potentiometer, etc.)
**Recommended Pins**: 32, 33, 34, 35, 36, 39 (ADC1 - works with WiFi)

### PWM Output (Motor, LED dimming, etc.)
**Recommended Pins**: 4, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33

### Interrupt Input (Encoder, Sensor, etc.)
**Recommended Pins**: 4, 16, 17, 18, 19, 21, 22, 23, 32, 33

## Pin Selection Guidelines

### Priority 1: Safe Pins (No Restrictions)
```
GPIO: 4, 16, 17, 18, 19, 21, 22, 23, 32, 33
```
Use these first for any application.

### Priority 2: ADC2 Pins (If Not Using Analog)
```
GPIO: 13, 14, 25, 26, 27
```
Safe for digital I/O if you don't need analog input.

### Priority 3: Strapping Pin GPIO 5
```
GPIO: 5
```
Generally safe but use after other options.

### Avoid Unless Necessary
```
GPIO: 0, 2, 12, 15 (Strapping pins)
GPIO: 1, 3 (Serial)
```

### Never Use
```
GPIO: 6, 7, 8, 9, 10, 11 (Flash)
```

## Hardware Design Tips

1. **Pull-up/Pull-down Resistors**
   - Add external pull-up (10kΩ) to GPIO 0 for reliable boot
   - Add external pull-down (10kΩ) to GPIO 2, 12, 15 for reliable boot
   - Input-only pins (34-39) need external pull-up/down if required

2. **Boot Reliability**
   - Ensure strapping pins are in correct state during boot
   - Use switches with pull-up/down resistors on strapping pins
   - Test boot behavior with all peripherals connected

3. **ADC Usage**
   - Use ADC1 (GPIO 32-39) for analog inputs in WiFi applications
   - ADC2 (GPIO 13, 14, 25-27) only works when WiFi is off

4. **Power Considerations**
   - Maximum current per GPIO: 12mA (recommended), 40mA (absolute max)
   - Use transistors or MOSFETs for high-current loads
   - Total current for all GPIOs: 200mA max

5. **Voltage Levels**
   - GPIO voltage: 3.3V (NOT 5V tolerant)
   - Use level shifters for 5V devices
   - Input voltage range: 0V to 3.6V

## Troubleshooting

### ESP32 Won't Boot
- Check GPIO 0 is HIGH (or floating with pull-up)
- Check GPIO 2 is LOW (or floating)
- Check GPIO 12 is LOW
- Check GPIO 15 is LOW
- Disconnect peripherals and test

### ADC Not Working
- If using ADC2, disable WiFi
- Use ADC1 (GPIO 32-39) for WiFi applications
- Check voltage is within 0-3.3V range

### GPIO Not Responding
- Check if pin is input-only (GPIO 34-39)
- Check if pin is flash pin (GPIO 6-11)
- Verify pin is not used by another peripheral
- Check for short circuits

## References

- [ESP32 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf)
- [ESP32 Technical Reference Manual](https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf)
- [ESP32 Hardware Design Guidelines](https://www.espressif.com/sites/default/files/documentation/esp32_hardware_design_guidelines_en.pdf)

## Conclusion

This reference guide provides comprehensive information about ESP32 WROOM-32 GPIO pins. Always consult this guide when designing hardware or configuring GPIO pins to ensure reliable operation and avoid boot issues.

For the Washka system, the pin legend is integrated into the web interface and provides real-time warnings and recommendations during configuration.
