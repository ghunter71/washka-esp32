/**
 * WaterControl.cpp
 * 
 * Implementation of gerkon-based water level control.
 */

#include "WaterControl.h"

// Global pointer for ISR
static WaterControl* waterControlInstance = nullptr;

// ISR wrapper
void IRAM_ATTR gerkonISR() {
    if (waterControlInstance) {
        waterControlInstance->handleGerkonInterrupt();
    }
}

WaterControl::WaterControl() :
    gerkonPin(0),
    gerkonCount(0),
    debounceMs(50),
    lastTriggerTime(0),
    initialized(false),
    triggerCallback(nullptr) {
}

WaterControl::~WaterControl() {
    if (initialized) {
        detachInterrupt(digitalPinToInterrupt(gerkonPin));
    }
}

bool WaterControl::begin(uint8_t pin) {
    gerkonPin = pin;
    
    // Configure pin as input with pull-up
    pinMode(gerkonPin, INPUT_PULLUP);
    
    // Reset counter
    gerkonCount = 0;
    lastTriggerTime = 0;
    
    // Set global instance for ISR
    waterControlInstance = this;
    
    // Attach interrupt on FALLING edge (HIGH -> LOW transition)
    // With pull-up: HIGH = open, LOW = closed (magnet detected)
    attachInterrupt(digitalPinToInterrupt(gerkonPin), gerkonISR, FALLING);
    
    initialized = true;
    
    Serial.println("✓ WaterControl initialized (FALLING edge detection)");
    return true;
}

// ============================================================================
// Water Filling Control
// ============================================================================

WaterControl::FillResult WaterControl::fillWater(uint16_t gerkonThreshold, unsigned long timeoutMs) {
    if (!initialized) {
        Serial.println("ERROR: WaterControl not initialized");
        return FillResult::ERROR_SENSOR;
    }
    
    Serial.printf("Starting water fill: threshold=%d, timeout=%lu ms\n", 
                  gerkonThreshold, timeoutMs);
    
    // Reset counter
    resetGerkonCount();
    
    unsigned long startTime = millis();
    unsigned long lastCountTime = startTime;
    uint16_t lastCount = 0;
    
    while (true) {
        unsigned long currentTime = millis();
        uint16_t currentCount = getGerkonCount();
        
        // Check if threshold reached
        if (currentCount >= gerkonThreshold) {
            Serial.printf("✓ Water fill complete: count=%d (threshold reached)\n", currentCount);
            return FillResult::SUCCESS_GERKON;
        }
        
        // Check for sensor activity
        if (currentCount > lastCount) {
            lastCountTime = currentTime;
            lastCount = currentCount;
        }
        
        // Check timeout
        if (currentTime - startTime >= timeoutMs) {
            if (currentCount > 0) {
                Serial.printf("⚠ Water fill timeout: count=%d (partial fill)\n", currentCount);
                return FillResult::SUCCESS_TIMEOUT;
            } else {
                Serial.println("ERROR: Water fill timeout with no sensor activity");
                return FillResult::ERROR_TIMEOUT;
            }
        }
        
        // Check for sensor malfunction (no activity for too long)
        if (currentTime - lastCountTime > (timeoutMs / 2) && currentCount == 0) {
            Serial.println("ERROR: Sensor malfunction detected (no activity)");
            return FillResult::ERROR_SENSOR;
        }
        
        delay(10); // Small delay to prevent busy-waiting
        yield();   // Allow other tasks to run
    }
}

// ============================================================================
// Gerkon Counter
// ============================================================================

uint16_t WaterControl::getGerkonCount() {
    return gerkonCount;
}

void WaterControl::resetGerkonCount() {
    gerkonCount = 0;
    lastTriggerTime = 0;
    Serial.println("Gerkon counter reset");
}

// ============================================================================
// Debouncing
// ============================================================================

void WaterControl::setDebounceMs(uint16_t ms) {
    debounceMs = ms;
    Serial.printf("Debounce time set to %d ms\n", ms);
}

uint16_t WaterControl::getDebounceMs() {
    return debounceMs;
}

// ============================================================================
// Callbacks
// ============================================================================

void WaterControl::onGerkonTrigger(GerkonTriggerCallback callback) {
    triggerCallback = callback;
}

// ============================================================================
// Interrupt Handler
// ============================================================================

void IRAM_ATTR WaterControl::handleGerkonInterrupt() {
    unsigned long currentTime = millis();
    
    // Debouncing: ignore triggers within debounce period
    if (currentTime - lastTriggerTime < debounceMs) {
        return;
    }
    
    // Update last trigger time
    lastTriggerTime = currentTime;
    
    // Increment counter
    gerkonCount++;
    
    // Call callback if set (be careful in ISR context)
    if (triggerCallback) {
        triggerCallback(gerkonCount);
    }
}

// ============================================================================
// Private Helper Methods
// ============================================================================

bool WaterControl::isGerkonTriggered() {
    return digitalRead(gerkonPin) == HIGH;
}
