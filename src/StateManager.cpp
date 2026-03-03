/**
 * StateManager.cpp
 * 
 * Implementation of wash cycle state machine.
 */

#include "StateManager.h"

StateManager::StateManager() :
    currentState(WashState::IDLE),
    previousState(WashState::IDLE),
    paused(false),
    cycleStartTime(0),
    stateStartTime(0),
    stateChangeCallback(nullptr) {
}

StateManager::~StateManager() {
}

bool StateManager::begin() {
    // Always start in IDLE state (no state recovery)
    currentState = WashState::IDLE;
    previousState = WashState::IDLE;
    paused = false;
    cycleStartTime = 0;
    stateStartTime = millis();
    
    Serial.println("✓ StateManager initialized in IDLE state");
    return true;
}

// ============================================================================
// State Control
// ============================================================================

WashState StateManager::getCurrentState() {
    return currentState;
}

void StateManager::setState(WashState newState) {
    if (currentState == newState) {
        return; // No change
    }
    
    WashState oldState = currentState;
    previousState = currentState;
    currentState = newState;
    stateStartTime = millis();
    
    Serial.printf("State transition: %s -> %s\n", 
                  getStateDescription(oldState).c_str(),
                  getStateDescription(newState).c_str());
    
    // Notify callback
    notifyStateChange(oldState, newState);
}

bool StateManager::isValidTransition(WashState from, WashState to) {
    // IDLE can transition to DRAIN_PREWASH (start cycle)
    if (from == WashState::IDLE && to == WashState::DRAIN_PREWASH) {
        return true;
    }
    
    // Any state can transition to IDLE (stop) or ERROR
    if (to == WashState::IDLE || to == WashState::ERROR) {
        return true;
    }
    
    // Sequential transitions in wash cycle
    if (static_cast<uint8_t>(to) == static_cast<uint8_t>(from) + 1) {
        return true;
    }
    
    // COMPLETE can transition to IDLE
    if (from == WashState::COMPLETE && to == WashState::IDLE) {
        return true;
    }
    
    return false;
}

// ============================================================================
// Cycle Control
// ============================================================================

bool StateManager::startCycle() {
    if (currentState != WashState::IDLE) {
        Serial.println("ERROR: Cannot start cycle - not in IDLE state");
        return false;
    }
    
    cycleStartTime = millis();
    setState(WashState::DRAIN_PREWASH);
    paused = false;
    
    Serial.println("✓ Wash cycle started");
    return true;
}

bool StateManager::stopCycle() {
    if (currentState == WashState::IDLE) {
        Serial.println("Already in IDLE state");
        return true;
    }
    
    WashState oldState = currentState;
    setState(WashState::IDLE);
    paused = false;
    cycleStartTime = 0;
    
    Serial.printf("✓ Wash cycle stopped from state: %s\n", 
                  getStateDescription(oldState).c_str());
    return true;
}

bool StateManager::pauseCycle() {
    if (currentState == WashState::IDLE || currentState == WashState::COMPLETE) {
        Serial.println("ERROR: Cannot pause - cycle not running");
        return false;
    }
    
    if (paused) {
        Serial.println("Already paused");
        return true;
    }
    
    paused = true;
    Serial.println("✓ Wash cycle paused");
    return true;
}

bool StateManager::resumeCycle() {
    if (!paused) {
        Serial.println("ERROR: Cannot resume - not paused");
        return false;
    }
    
    paused = false;
    Serial.println("✓ Wash cycle resumed");
    return true;
}

// ============================================================================
// State Information
// ============================================================================

String StateManager::getStateDescription() {
    return getStateDescription(currentState);
}

String StateManager::getStateDescription(WashState state) {
    switch (state) {
        case WashState::IDLE:
            return "Idle";
        case WashState::DRAIN_PREWASH:
            return "Draining (Pre-wash)";
        case WashState::FILL_PREWASH:
            return "Filling (Pre-wash)";
        case WashState::PREWASH:
            return "Pre-washing";
        case WashState::DRAIN_WASH:
            return "Draining (Wash)";
        case WashState::FILL_WASH:
            return "Filling (Wash)";
        case WashState::WASH:
            return "Washing";
        case WashState::DRAIN_RINSE1:
            return "Draining (Rinse 1)";
        case WashState::FILL_RINSE1:
            return "Filling (Rinse 1)";
        case WashState::RINSE1:
            return "Rinsing (1)";
        case WashState::DRAIN_RINSE2:
            return "Draining (Rinse 2)";
        case WashState::FILL_RINSE2:
            return "Filling (Rinse 2)";
        case WashState::RINSE2:
            return "Rinsing (2)";
        case WashState::FINAL_DRAIN:
            return "Final Drain";
        case WashState::COMPLETE:
            return "Complete";
        case WashState::ERROR:
            return "Error";
        default:
            return "Unknown";
    }
}

uint8_t StateManager::getProgressPercent() {
    return calculateProgress();
}

unsigned long StateManager::getElapsedTime() {
    if (cycleStartTime == 0) {
        return 0;
    }
    return millis() - cycleStartTime;
}

unsigned long StateManager::getEstimatedRemaining() {
    return estimateRemainingTime();
}

// ============================================================================
// State Queries
// ============================================================================

bool StateManager::isIdle() {
    return currentState == WashState::IDLE;
}

bool StateManager::isRunning() {
    return currentState != WashState::IDLE && 
           currentState != WashState::COMPLETE && 
           currentState != WashState::ERROR &&
           !paused;
}

bool StateManager::isPaused() {
    return paused;
}

bool StateManager::isComplete() {
    return currentState == WashState::COMPLETE;
}

bool StateManager::isError() {
    return currentState == WashState::ERROR;
}

bool StateManager::isCriticalState() {
    // Critical states where OTA should be blocked
    switch (currentState) {
        case WashState::WASH:
        case WashState::DRAIN_PREWASH:
        case WashState::DRAIN_WASH:
        case WashState::DRAIN_RINSE1:
        case WashState::DRAIN_RINSE2:
        case WashState::FINAL_DRAIN:
        case WashState::FILL_PREWASH:
        case WashState::FILL_WASH:
        case WashState::FILL_RINSE1:
        case WashState::FILL_RINSE2:
            return true;
        default:
            return false;
    }
}

// ============================================================================
// Callbacks
// ============================================================================

void StateManager::onStateChange(StateChangeCallback callback) {
    stateChangeCallback = callback;
}

void StateManager::notifyStateChange(WashState oldState, WashState newState) {
    if (stateChangeCallback) {
        stateChangeCallback(oldState, newState);
    }
}

// ============================================================================
// Timing
// ============================================================================

void StateManager::setCycleStartTime(unsigned long startTime) {
    cycleStartTime = startTime;
}

unsigned long StateManager::getCycleStartTime() {
    return cycleStartTime;
}

// ============================================================================
// Private Helper Methods
// ============================================================================

uint8_t StateManager::calculateProgress() {
    if (currentState == WashState::IDLE) {
        return 0;
    }
    
    if (currentState == WashState::COMPLETE) {
        return 100;
    }
    
    // Approximate progress based on state
    // Total states in cycle: DRAIN_PREWASH(2) to COMPLETE(15) = 14 states
    uint8_t stateNum = static_cast<uint8_t>(currentState);
    
    if (stateNum >= 2 && stateNum <= 15) {
        // Map state 2-15 to progress 0-100%
        return ((stateNum - 2) * 100) / 13;
    }
    
    return 0;
}

unsigned long StateManager::estimateRemainingTime() {
    if (currentState == WashState::IDLE || currentState == WashState::COMPLETE) {
        return 0;
    }
    
    // Rough estimate based on typical cycle times
    // This would be more accurate with actual timing configuration
    unsigned long totalEstimate = 7200000; // ~2 hours typical cycle
    unsigned long elapsed = getElapsedTime();
    uint8_t progress = getProgressPercent();
    
    if (progress == 0) {
        return totalEstimate;
    }
    
    unsigned long estimatedTotal = (elapsed * 100) / progress;
    unsigned long remaining = estimatedTotal - elapsed;
    
    return remaining;
}
