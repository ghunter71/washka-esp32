# OTA Property Test Documentation

## Property 18: OTA Critical State Blocking

**Feature:** esp32-washka-upgrade  
**Validates:** Requirements 11.5  
**Test File:** `test/test_ota_updater.cpp`

### Test Description

This property-based test verifies that OTA (Over-The-Air) firmware updates are properly blocked during critical system states to prevent dangerous interruptions during wash cycles.

### Property Statement

*For any* system state marked as critical (e.g., WASH, DRAIN, FILL), OTA update initiation should be blocked.

### Critical States (OTA Blocked)

The following states are considered critical and will block OTA updates:
- `WASH` - Main wash phase
- `DRAIN_PREWASH` - Draining before pre-wash
- `DRAIN_WASH` - Draining before main wash
- `DRAIN_RINSE1` - Draining before first rinse
- `DRAIN_RINSE2` - Draining before second rinse
- `FINAL_DRAIN` - Final drain phase
- `FILL_PREWASH` - Filling water for pre-wash
- `FILL_WASH` - Filling water for main wash
- `FILL_RINSE1` - Filling water for first rinse
- `FILL_RINSE2` - Filling water for second rinse

### Non-Critical States (OTA Allowed)

The following states are safe for OTA updates:
- `IDLE` - System waiting for commands
- `PREWASH` - Pre-wash/soak phase
- `RINSE1` - First rinse phase
- `RINSE2` - Second rinse phase
- `COMPLETE` - Cycle complete
- `ERROR` - Error state

### Test Implementation

The test includes four test functions:

#### 1. `property_test_ota_critical_state_blocking()` (Main Property Test)
- Runs 100 iterations with randomly generated states
- For each iteration:
  - Tests a random critical state → verifies OTA is blocked
  - Tests a random non-critical state → verifies OTA is allowed
- Validates that ALL critical states were blocked (100/100)
- Validates that ALL non-critical states were allowed (100/100)

#### 2. `property_test_ota_blocks_during_update()`
- Verifies that once an update starts, subsequent updates are blocked
- Tests the `isUpdating()` flag behavior

#### 3. `property_test_all_critical_states_blocked()`
- Exhaustive test of all 10 critical states
- Verifies each one individually blocks OTA

#### 4. `property_test_all_noncritical_states_allowed()`
- Exhaustive test of all 6 non-critical states
- Verifies each one individually allows OTA

### Running the Tests

To run the OTA property tests:

```bash
# Run all OTA tests
pio test -e native -f test_ota_updater

# Run with verbose output
pio test -e native -f test_ota_updater -v
```

### Expected Output

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

### Implementation Details

The test uses:
- **Random generators** for property-based testing
- **100 iterations** as specified in the design document
- **Exhaustive testing** of all states for completeness
- **Unity test framework** for assertions
- **Mock AsyncWebServer** for testing without hardware

### Safety Rationale

Blocking OTA during critical states prevents:
1. **Water damage** - Interrupting fill/drain operations
2. **Motor damage** - Interrupting wash cycles
3. **Incomplete cycles** - Leaving dishes partially washed
4. **State corruption** - System reboot during state transitions

OTA is only allowed when the system is:
- Idle and waiting for commands
- In stable wash/rinse phases (not transitioning)
- Complete or in error state (safe to update)

### Related Files

- `include/OTAUpdater.h` - OTA updater interface
- `src/OTAUpdater.cpp` - OTA updater implementation with `canUpdate()` safety check
- `include/StateManager.h` - State machine with `isCriticalState()` method
- `src/StateManager.cpp` - State machine implementation
- `.kiro/specs/esp32-washka-upgrade/design.md` - Design document with Property 18 definition
- `.kiro/specs/esp32-washka-upgrade/requirements.md` - Requirements document with Requirement 11.5
