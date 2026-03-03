/**
 * ActuatorManager.cpp
 * 
 * Implementation of hardware actuator management with safety checks.
 */

#include "ActuatorManager.h"

ActuatorManager::ActuatorManager() :
    initialized(false) {
}

ActuatorManager::~ActuatorManager() {
    if (initialized) {
        emergencyStop();
    }
}

bool ActuatorManager::begin(const ConfigManager::PinConfig& pins) {
    pinConfig = pins;
    
    // Configure all pins as outputs
    pinMode(pinConfig.washengine, OUTPUT);
    pinMode(pinConfig.pompa, OUTPUT);
    pinMode(pinConfig.water_valve, OUTPUT);
    pinMode(pinConfig.powder, OUTPUT);
    pinMode(pinConfig.led, OUTPUT);
    
    // Initialize all actuators to OFF state
    digitalWrite(pinConfig.washengine, LOW);
    digitalWrite(pinConfig.pompa, LOW);
    digitalWrite(pinConfig.water_valve, LOW);
    digitalWrite(pinConfig.powder, LOW);
    digitalWrite(pinConfig.led, LOW);
    
    // Update status
    currentStatus.washengine = false;
    currentStatus.pompa = false;
    currentStatus.water_valve = false;
    currentStatus.powder = false;
    currentStatus.led = false;
    
    initialized = true;
    
    Serial.println("✓ ActuatorManager initialized");
    return true;
}

// ============================================================================
// Individual Actuator Control
// ============================================================================

bool ActuatorManager::setWashEngine(bool state) {
    if (!initialized) {
        Serial.println("ERROR: ActuatorManager not initialized");
        return false;
    }
    
    setActuatorState(pinConfig.washengine, state);
    currentStatus.washengine = state;
    
    Serial.printf("Wash engine: %s\n", state ? "ON" : "OFF");
    return true;
}

bool ActuatorManager::setPump(bool state) {
    if (!initialized) {
        Serial.println("ERROR: ActuatorManager not initialized");
        return false;
    }
    
    // Safety check: cannot turn on pump if water valve is on
    if (state && currentStatus.water_valve) {
        Serial.println("ERROR: Cannot turn on pump while water valve is on (mutual exclusion)");
        return false;
    }
    
    setActuatorState(pinConfig.pompa, state);
    currentStatus.pompa = state;
    
    Serial.printf("Pump: %s\n", state ? "ON" : "OFF");
    return true;
}

bool ActuatorManager::setWaterValve(bool state) {
    if (!initialized) {
        Serial.println("ERROR: ActuatorManager not initialized");
        return false;
    }
    
    // Safety check: cannot turn on water valve if pump is on
    if (state && currentStatus.pompa) {
        Serial.println("ERROR: Cannot turn on water valve while pump is on (mutual exclusion)");
        return false;
    }
    
    setActuatorState(pinConfig.water_valve, state);
    currentStatus.water_valve = state;
    
    Serial.printf("Water valve: %s\n", state ? "ON" : "OFF");
    return true;
}

bool ActuatorManager::setPowderDispenser(bool state) {
    if (!initialized) {
        Serial.println("ERROR: ActuatorManager not initialized");
        return false;
    }
    
    setActuatorState(pinConfig.powder, state);
    currentStatus.powder = state;
    
    Serial.printf("Powder dispenser: %s\n", state ? "ON" : "OFF");
    return true;
}

bool ActuatorManager::setLED(bool state) {
    if (!initialized) {
        Serial.println("ERROR: ActuatorManager not initialized");
        return false;
    }
    
    setActuatorState(pinConfig.led, state);
    currentStatus.led = state;
    
    return true;
}

// ============================================================================
// Safety Checks
// ============================================================================

bool ActuatorManager::canFillWater() {
    // Can fill water only if pump is off
    return !currentStatus.pompa;
}

bool ActuatorManager::canDrain() {
    // Can drain only if water valve is off
    return !currentStatus.water_valve;
}

// ============================================================================
// Emergency Stop
// ============================================================================

void ActuatorManager::emergencyStop() {
    if (!initialized) {
        return;
    }
    
    Serial.println("!!! EMERGENCY STOP - Disabling all actuators !!!");
    
    // Immediately turn off all actuators
    digitalWrite(pinConfig.washengine, LOW);
    digitalWrite(pinConfig.pompa, LOW);
    digitalWrite(pinConfig.water_valve, LOW);
    digitalWrite(pinConfig.powder, LOW);
    digitalWrite(pinConfig.led, LOW);
    
    // Update status
    currentStatus.washengine = false;
    currentStatus.pompa = false;
    currentStatus.water_valve = false;
    currentStatus.powder = false;
    currentStatus.led = false;
    
    Serial.println("✓ All actuators disabled");
}

// ============================================================================
// Status
// ============================================================================

ActuatorManager::ActuatorStatus ActuatorManager::getStatus() {
    return currentStatus;
}

// ============================================================================
// Private Helper Methods
// ============================================================================

void ActuatorManager::setActuatorState(uint8_t pin, bool state) {
    digitalWrite(pin, state ? HIGH : LOW);
}

bool ActuatorManager::getActuatorState(uint8_t pin) {
    return digitalRead(pin) == HIGH;
}
