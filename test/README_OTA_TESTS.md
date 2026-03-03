# OTA Updater Tests

## Overview

This document describes the property-based tests for the OTAUpdater component, which manages Over-The-Air firmware updates with safety checks.

## Test File

- `test/test_ota_updater.cpp` - Property-based tests for OTA critical state blocking

## Property Tests

### Property 18: OTA Critical State Blocking

**Validates:** Requirements 11.5

**Description:** For any system state marked as critical (e.g., WASH, DRAIN, FILL), OTA update initiation should be blocked.

**Test Implementation:**
- Generates random critical and non-critical states
- Verifies that `canUpdate()` returns `false` for all critical states
- Verifies that `canUpdate()` returns `true` for all non-critical states (IDLE, COMPLETE, ERROR)
- Runs 100 iterations to ensure property holds across all state combinations

**Critical States (OTA Blocked):**
- WASH
- DRAIN_PREWASH
- DRAIN_WASH
- DRAIN_RINSE1
- DRAIN_RINSE2
- FINAL_DRAIN
- FILL_PREWASH
- FILL_WASH
- FILL_RINSE1
- FILL_RINSE2

**Non-Critical States (OTA Allowed):**
- IDLE
- PREWASH
- RINSE1
- RINSE2
- COMPLETE
- ERROR

## Running Tests

### On ESP32 Hardware

To run the OTA tests on actual ESP32 hardware:

```bash
# Upload and run tests
pio test -e esp32dev -f test_ota_updater

# Or upload and monitor
pio test -e esp32dev -f test_ota_updater --upload-port /dev/ttyUSB0 --test-port /dev/ttyUSB0
```

### Test Output

Expected output for passing tests:

```
=== Property Test: OTA Critical State Blocking ===
  Critical states blocked: 100/100
  Non-critical states allowed: 100/100
✓ Property 18 passed: OTA critical state blocking

=== Property Test: OTA Blocks During Update ===
✓ OTA blocks during update test passed

=== Property Test: All Critical States Blocked ===
  ✓ Blocked: Washing
  ✓ Blocked: Draining (Pre-wash)
  ✓ Blocked: Draining (Wash)
  ✓ Blocked: Draining (Rinse 1)
  ✓ Blocked: Draining (Rinse 2)
  ✓ Blocked: Final Drain
  ✓ Blocked: Filling (Pre-wash)
  ✓ Blocked: Filling (Wash)
  ✓ Blocked: Filling (Rinse 1)
  ✓ Blocked: Filling (Rinse 2)
✓ All critical states blocked test passed

=== Property Test: All Non-Critical States Allowed ===
  ✓ Allowed: Idle
  ✓ Allowed: Pre-washing
  ✓ Allowed: Rinsing (1)
  ✓ Allowed: Rinsing (2)
  ✓ Allowed: Complete
  ✓ Allowed: Error
✓ All non-critical states allowed test passed
```

## Manual Testing

### Testing OTA Update Flow

1. **Access OTA Update Page:**
   - Navigate to `http://<device-ip>/update`
   - Verify the page loads with system status

2. **Test Critical State Blocking:**
   - Start a wash cycle via web interface or API
   - Try to access `/update` during WASH state
   - Should see: "OTA update blocked: System is in critical state"

3. **Test Successful Update:**
   - Ensure system is in IDLE state
   - Access `/update` page
   - Select a valid firmware `.bin` file
   - Click "Upload Firmware"
   - Verify progress bar updates
   - Device should reboot after successful update

4. **Test Update Rejection:**
   - Try uploading an invalid file (not .bin)
   - Should see error message
   - System should remain stable

### Safety Verification

1. **During Active Cycle:**
   ```bash
   # Start wash cycle
   curl -X POST http://<device-ip>/api/control/start
   
   # Try to access OTA (should be blocked)
   curl http://<device-ip>/update
   # Expected: 503 Service Unavailable
   ```

2. **In IDLE State:**
   ```bash
   # Ensure IDLE state
   curl -X POST http://<device-ip>/api/control/stop
   
   # Access OTA (should be allowed)
   curl http://<device-ip>/update
   # Expected: 200 OK with HTML page
   ```

## Implementation Notes

### Safety Checks

The OTAUpdater implements the following safety checks:

1. **State Check:** Blocks updates during critical states (WASH, DRAIN, FILL)
2. **Concurrent Update Check:** Prevents multiple simultaneous updates
3. **State Manager Integration:** Uses StateManager.isCriticalState() for safety

### Update Process

1. User accesses `/update` endpoint
2. System checks `canUpdate()`:
   - Returns false if in critical state
   - Returns false if update already in progress
   - Returns true if in IDLE, COMPLETE, or ERROR state
3. If allowed, serves OTA update page
4. User uploads firmware file
5. System validates and applies update
6. Device reboots with new firmware

### Error Handling

- Invalid firmware: Update rejected, error logged
- Update during critical state: HTTP 503 returned
- Update failure: Rollback to previous firmware (ESP32 OTA partition feature)
- Network interruption: Update aborted, system remains stable

## Test Coverage

The property tests provide comprehensive coverage:

- ✓ All critical states block OTA
- ✓ All non-critical states allow OTA
- ✓ Random state combinations (100 iterations)
- ✓ Exhaustive testing of all defined states
- ✓ Concurrent update prevention

## Troubleshooting

### Test Fails to Compile

Ensure all dependencies are installed:
```bash
pio lib install
```

### Test Fails on Hardware

1. Check serial output for detailed error messages
2. Verify StateManager is properly initialized
3. Ensure AsyncWebServer is available
4. Check that Update library is included

### OTA Update Fails

1. Verify firmware file is valid ESP32 binary
2. Check available flash space
3. Ensure stable WiFi connection
4. Review serial logs for Update library errors

## Related Files

- `include/OTAUpdater.h` - OTAUpdater class definition
- `src/OTAUpdater.cpp` - OTAUpdater implementation
- `include/StateManager.h` - State management (critical state detection)
- `src/main.cpp` - OTAUpdater integration

## Requirements Validation

This test validates the following requirements:

- **Requirement 11.5:** WHEN system is performing critical operations THEN the OTAUpdater SHALL prevent update initiation
- **Property 18:** For any system state marked as critical, OTA update initiation should be blocked
