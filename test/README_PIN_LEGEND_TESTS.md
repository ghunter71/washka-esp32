# Pin Legend Tests

## Overview

Unit tests for the WROOM-32 Pin Legend system that validates pin information, capabilities, restrictions, and recommendations.

## Test File

- `test_pin_legend.cpp` - Comprehensive unit tests for PinLegend functionality

## Test Coverage

### 1. Pin Validation Tests (`test_pin_validation`)
- Validates that safe pins (4, 32, 33) are marked as valid
- Validates that flash pins (6-11) are marked as invalid
- Validates that out-of-range pins (40+) are marked as invalid

### 2. Pin Recommendation Tests (`test_pin_recommendations`)
- Verifies recommended pins (4, 32, 33) are marked as recommended
- Verifies strapping pins (0, 2, 12, 15) are not recommended
- Verifies flash pins (6-11) are not recommended

### 3. Pin Warning Tests (`test_pin_warnings`)
- Verifies strapping pins have warnings
- Verifies flash pins have warnings
- Verifies safe pins have no warnings

### 4. Pin Info Retrieval Tests (`test_pin_info`)
- Tests retrieval of pin information for GPIO 32 (safe pin)
- Tests retrieval for GPIO 34 (input-only pin)
- Tests retrieval for GPIO 0 (strapping pin)
- Tests retrieval for GPIO 6 (flash pin)
- Validates pin properties (recommended, canUseForOutput, isInputOnly, etc.)

### 5. Pin Capability Tests (`test_pin_capabilities`)
- Tests ADC1 capability detection (GPIO 32)
- Tests DAC capability detection (GPIO 25)
- Tests ADC2 capability detection (GPIO 25)
- Tests input-only capability detection (GPIO 34)

### 6. Recommended Pins List Tests (`test_recommended_pins_list`)
- Verifies list contains multiple recommended pins
- Verifies list includes known safe pins (4, 32, 33)
- Verifies list excludes flash pins (6-11)
- Verifies list excludes input-only pins (34-39)

### 7. JSON Export Tests (`test_json_export`)
- Verifies JSON export is not empty
- Verifies JSON contains expected keys (pins, gpio, label, etc.)
- Validates JSON structure

### 8. Pin Description Tests (`test_pin_description`)
- Tests description generation for GPIO 32
- Tests description includes capabilities (ADC1)
- Tests description for GPIO 25 includes DAC capability

## Running Tests

### Native Environment (Recommended)

```bash
# Run all tests
pio test -e native

# Run only pin legend tests
pio test -e native -f test_pin_legend
```

### ESP32 Environment

```bash
# Run tests on ESP32 hardware
pio test -e esp32dev -f test_pin_legend
```

## Expected Results

All tests should pass:

```
test_pin_validation: PASS
test_pin_recommendations: PASS
test_pin_warnings: PASS
test_pin_info: PASS
test_pin_capabilities: PASS
test_recommended_pins_list: PASS
test_json_export: PASS
test_pin_description: PASS

8 Tests 0 Failures 0 Ignored
OK
```

## Test Details

### Pin Categories Tested

1. **Safe Pins**: GPIO 4, 32, 33
   - Should be valid
   - Should be recommended
   - Should have no warnings
   - Should be usable for output

2. **Strapping Pins**: GPIO 0, 2, 12, 15
   - Should be valid but not recommended
   - Should have warnings
   - Should be marked as strapping

3. **Flash Pins**: GPIO 6-11
   - Should be invalid
   - Should not be recommended
   - Should have warnings
   - Should be marked as flash pins

4. **Input-Only Pins**: GPIO 34-39
   - Should be valid
   - Should be recommended (for input use)
   - Should be marked as input-only
   - Should not be usable for output

5. **Special Capability Pins**:
   - GPIO 32: ADC1, Touch, RTC
   - GPIO 25: ADC2, DAC, RTC
   - GPIO 34: ADC1, Input Only

## Pin Information Validated

For each pin, the tests validate:
- GPIO number
- Label (e.g., "D32")
- Description (e.g., "GPIO32/ADC1_4")
- Recommended status
- Warning text
- Capabilities (ADC1, ADC2, DAC, Touch, RTC, Digital I/O, Input Only)
- Restrictions (Strapping, Flash, Serial, Input Only, ADC2/WiFi)
- Helper flags (canUseForOutput, isInputOnly, isStrapping, isFlashPin)

## Integration with ConfigManager

The PinLegend is integrated with ConfigManager for validation:

```cpp
bool ConfigManager::isPinValid(uint8_t pin) {
    return PinLegend::isPinValid(pin);
}
```

This ensures that:
- Invalid pins are rejected during configuration
- Flash pins cannot be assigned
- Pin validation is consistent across the system

## API Integration

The pin legend is exposed via REST API:

```
GET /api/pins
```

Returns comprehensive pin information in JSON format for use by the web interface.

## Web UI Integration

The web interface (`config-gpio.html`) uses the pin legend to:
1. Populate pin dropdowns with valid options
2. Color-code pins based on recommendations
3. Display warnings for problematic pins
4. Show detailed pin information in sidebar
5. Prevent invalid pin assignments

## Troubleshooting

### Test Failures

If tests fail, check:
1. PinLegend.cpp has correct pin definitions
2. Pin capabilities are correctly set
3. Pin restrictions are correctly set
4. Recommended flags are correctly set

### Common Issues

1. **Flash pin validation**: Ensure GPIO 6-11 are marked as invalid
2. **Strapping pin warnings**: Ensure GPIO 0, 2, 12, 15 have warnings
3. **Input-only detection**: Ensure GPIO 34-39 are marked as input-only
4. **Capability detection**: Ensure ADC1, ADC2, DAC capabilities are correct

## Requirements Validated

These tests validate implementation of:
- **Requirement 3.5**: Pin configuration with WROOM-32 legend
- **Requirement 9.1**: Display pin legend in configuration UI
- **Requirement 9.5**: Display warnings for invalid pin selections

## Future Test Enhancements

Potential additional tests:
1. Pin conflict detection (e.g., I2C vs GPIO)
2. ADC2/WiFi conflict validation
3. Pin usage history tracking
4. Hardware profile validation
5. Pin assignment optimization
6. Multi-pin configuration validation

## Conclusion

The pin legend tests provide comprehensive validation of the WROOM-32 pin information system, ensuring accurate pin data, proper validation, and safe pin assignments.
