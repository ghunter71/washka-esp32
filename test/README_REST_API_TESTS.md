# RestAPI Property-Based Tests

## Overview

This document describes the property-based tests for the RestAPI component of the Washka ESP32 system.

## Test Files

- `test_rest_api.cpp` - Contains all property-based tests for RestAPI

## Running Tests

The tests require ESP32 hardware or an emulator to run. To execute the tests:

```bash
# Upload and run tests on connected ESP32 device
pio test -e esp32dev -f test_rest_api

# View test output via serial monitor
pio device monitor
```

## Property Tests Implemented

### Property 8: API JSON Response Validity
**Validates: Requirements 7.2**

Tests that all API endpoint responses return valid JSON with appropriate HTTP status codes.

**Test Strategy:**
- Generates 100 random system states
- Builds status and config JSON responses
- Verifies JSON validity using ArduinoJson parser
- Checks for required fields in responses
- Validates HTTP status codes

**Properties Verified:**
- All API responses must be valid JSON
- Status responses must contain: status, sensors, actuators, system fields
- Config responses must contain: pins, timing, gerkon fields
- Responses must use appropriate content-type headers

### Property 9: API Authentication Enforcement
**Validates: Requirements 7.3**

Tests that API authentication correctly accepts valid tokens and rejects invalid ones.

**Test Strategy:**
- Generates 100 random API tokens
- Tests authentication with valid tokens
- Tests rejection of invalid tokens
- Tests behavior when authentication is disabled

**Properties Verified:**
- Requests with valid Bearer tokens must be authenticated
- Requests with invalid tokens must return 401 Unauthorized
- Requests without tokens must fail when auth is enabled
- When authentication is disabled, all requests pass

### Property 10: API Parameter Validation
**Validates: Requirements 7.5**

Tests that API endpoints properly validate input parameters and reject invalid values.

**Test Strategy:**
- Generates 100 random configuration JSON payloads
- Tests pin validation (range 0-39, excluding 6-11)
- Tests timing validation (1000-86400000 ms)
- Tests gerkon threshold validation (1-1000)
- Tests gerkon debounce validation (1-1000 ms)
- Tests explicit invalid cases (out-of-range, flash pins, duplicates)
- Tracks valid and invalid parameter counts

**Properties Verified:**
- Invalid JSON must be rejected with 400 Bad Request
- Pin values outside valid range (0-39) must be rejected
- Flash pins (6-11) must be rejected
- Duplicate pin assignments must be rejected
- Timing values outside valid range (1000-86400000 ms) must be rejected
- Gerkon threshold outside valid range (1-1000) must be rejected
- Gerkon debounce outside valid range (1-1000 ms) must be rejected
- Valid parameters must be accepted and persisted correctly
- Invalid parameters must not corrupt system state

## Test Results

All tests compile successfully and are ready for execution on ESP32 hardware.

## Implementation Notes

The tests use the Unity test framework and follow the property-based testing methodology:
- Each test runs 100 iterations with random inputs
- Tests verify universal properties that must hold for all valid inputs
- Tests are tagged with feature name and property number for traceability

## Dependencies

- Unity test framework
- ArduinoJson for JSON parsing
- ESP32 Arduino framework
- ESPAsyncWebServer library
