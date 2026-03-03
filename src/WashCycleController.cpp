/**
 * WashCycleController.cpp
 * 
 * Implementation of automatic wash cycle execution.
 */

#include "WashCycleController.h"

WashCycleController::WashCycleController(StateManager* state, 
                                       ActuatorManager* actuators,
                                       WaterControl* water,
                                       ConfigManager* config) :
    stateManager(state),
    actuatorManager(actuators),
    waterControl(water),
    configManager(config),
    stateStartTime(0),
    stateTimeout(0),
    gerkonStartCount(0),
    gerkonThreshold(0),
    stateInitialized(false),
    powderActive(false),
    powderStartTime(0) {
}

WashCycleController::~WashCycleController() {
}

bool WashCycleController::begin() {
    // Load timing configuration
    timingConfig = configManager->getTimingConfig();
    gerkonThreshold = configManager->getGerkonThreshold();
    
    Serial.println("✓ WashCycleController initialized");
    return true;
}

// ============================================================================
// Main Loop
// ============================================================================

void WashCycleController::loop() {
    // Don't execute if paused
    if (stateManager->isPaused()) {
        return;
    }
    
    WashState currentState = stateManager->getCurrentState();
    
    // Handle powder dispenser auto-off (1 second after activation)
    if (powderActive && (millis() - powderStartTime >= POWDER_DURATION)) {
        actuatorManager->setPowderDispenser(false);
        powderActive = false;
        Serial.println("Powder dispenser auto-off (1 second elapsed)");
    }
    
    // Handle IDLE, ERROR states
    if (currentState == WashState::IDLE || currentState == WashState::ERROR) {
        // Stop all actuators when entering idle/error state
        if (stateInitialized) {
            stopAllActuators();
            powderActive = false;
            stateInitialized = false;
        }
        return;
    }
    
    // Handle COMPLETE state - wait 5 seconds then transition to IDLE
    if (currentState == WashState::COMPLETE) {
        if (!stateInitialized) {
            stopAllActuators();
            powderActive = false;
            stateStartTime = millis();
            stateTimeout = 5000;  // Wait 5 seconds before going to IDLE
            stateInitialized = true;
            Serial.println("Cycle complete - will return to IDLE in 5 seconds");
        }
        
        // Check if 5 seconds have passed
        if (millis() - stateStartTime >= stateTimeout) {
            Serial.println("Transitioning from Complete to Idle");
            stateManager->setState(WashState::IDLE);
            stateInitialized = false;
        }
        return;
    }
    
    // Initialize state on first entry
    if (!stateInitialized) {
        executeCurrentState();
        stateInitialized = true;
    }
    
    // Check if state is complete
    if (checkStateCompletion()) {
        stopAllActuators();
        powderActive = false;
        transitionToNextState();
        stateInitialized = false;
    }
}

// ============================================================================
// State Execution
// ============================================================================

void WashCycleController::executeCurrentState() {
    WashState state = stateManager->getCurrentState();
    stateStartTime = millis();
    stateTimeout = getStateTimeout(state);
    
    Serial.printf("Executing state: %s (timeout: %lu ms)\n", 
                  stateManager->getStateDescription(state).c_str(),
                  stateTimeout);
    
    switch (state) {
        case WashState::DRAIN_PREWASH:
        case WashState::DRAIN_WASH:
        case WashState::DRAIN_RINSE1:
        case WashState::DRAIN_RINSE2:
        case WashState::FINAL_DRAIN:
            startDrainPhase();
            break;
            
        case WashState::FILL_PREWASH:
        case WashState::FILL_RINSE1:
        case WashState::FILL_RINSE2:
            startFillPhase(false);
            break;
            
        case WashState::FILL_WASH:
            startFillPhase(true);  // Add powder during main wash fill
            break;
            
        case WashState::PREWASH:
        case WashState::WASH:
        case WashState::RINSE1:
        case WashState::RINSE2:
            startWashPhase();
            break;
            
        default:
            Serial.println("WARNING: Unknown state in executeCurrentState");
            break;
    }
}

bool WashCycleController::checkStateCompletion() {
    WashState state = stateManager->getCurrentState();
    
    switch (state) {
        case WashState::DRAIN_PREWASH:
        case WashState::DRAIN_WASH:
        case WashState::DRAIN_RINSE1:
        case WashState::DRAIN_RINSE2:
        case WashState::FINAL_DRAIN:
            return isDrainComplete();
            
        case WashState::FILL_PREWASH:
        case WashState::FILL_WASH:
        case WashState::FILL_RINSE1:
        case WashState::FILL_RINSE2:
            return isFillComplete();
            
        case WashState::PREWASH:
        case WashState::WASH:
        case WashState::RINSE1:
        case WashState::RINSE2:
            return isWashComplete();
            
        default:
            return false;
    }
}

void WashCycleController::transitionToNextState() {
    WashState currentState = stateManager->getCurrentState();
    WashState nextState = getNextState(currentState);
    
    Serial.printf("Transitioning: %s -> %s\n",
                  stateManager->getStateDescription(currentState).c_str(),
                  stateManager->getStateDescription(nextState).c_str());
    
    stateManager->setState(nextState);
}

// ============================================================================
// Phase Starters
// ============================================================================

void WashCycleController::startDrainPhase() {
    Serial.println("Starting DRAIN phase - Pump ON");
    actuatorManager->setPump(true);
    actuatorManager->setWashEngine(false);
    actuatorManager->setWaterValve(false);
    actuatorManager->setPowderDispenser(false);
}

void WashCycleController::startFillPhase(bool addPowder) {
    Serial.printf("Starting FILL phase - Valve ON%s\n", 
                  addPowder ? " + Powder (1 sec)" : "");
    
    // Reset gerkon counter
    waterControl->resetGerkonCount();
    gerkonStartCount = waterControl->getGerkonCount();
    
    actuatorManager->setPump(false);
    actuatorManager->setWashEngine(false);
    actuatorManager->setWaterValve(true);
    
    // Activate powder dispenser for 1 second if needed
    if (addPowder) {
        actuatorManager->setPowderDispenser(true);
        powderActive = true;
        powderStartTime = millis();
        Serial.println("Powder dispenser activated (will auto-off in 1 second)");
    } else {
        actuatorManager->setPowderDispenser(false);
        powderActive = false;
    }
}

void WashCycleController::startWashPhase() {
    Serial.println("Starting WASH phase - Motor ON");
    actuatorManager->setPump(false);
    actuatorManager->setWashEngine(true);
    actuatorManager->setWaterValve(false);
    actuatorManager->setPowderDispenser(false);
}

void WashCycleController::stopAllActuators() {
    actuatorManager->setPump(false);
    actuatorManager->setWashEngine(false);
    actuatorManager->setWaterValve(false);
    actuatorManager->setPowderDispenser(false);
}

// ============================================================================
// Completion Checks
// ============================================================================

bool WashCycleController::isDrainComplete() {
    unsigned long elapsed = millis() - stateStartTime;
    
    if (elapsed >= stateTimeout) {
        Serial.printf("DRAIN complete (timeout: %lu ms)\n", elapsed);
        return true;
    }
    
    return false;
}

bool WashCycleController::isFillComplete() {
    unsigned long elapsed = millis() - stateStartTime;
    
    // Check gerkon threshold
    int currentCount = waterControl->getGerkonCount();
    int countDelta = currentCount - gerkonStartCount;
    
    if (countDelta >= gerkonThreshold) {
        Serial.printf("FILL complete (gerkon: %d/%d, time: %lu ms)\n", 
                     countDelta, gerkonThreshold, elapsed);
        return true;
    }
    
    // Check timeout
    if (elapsed >= stateTimeout) {
        Serial.printf("FILL complete (timeout: %lu ms, gerkon: %d/%d)\n", 
                     elapsed, countDelta, gerkonThreshold);
        return true;
    }
    
    return false;
}

bool WashCycleController::isWashComplete() {
    unsigned long elapsed = millis() - stateStartTime;
    
    if (elapsed >= stateTimeout) {
        Serial.printf("WASH complete (timeout: %lu ms)\n", elapsed);
        return true;
    }
    
    return false;
}

// ============================================================================
// Helper Methods
// ============================================================================

WashState WashCycleController::getNextState(WashState current) {
    switch (current) {
        case WashState::DRAIN_PREWASH:
            return WashState::FILL_PREWASH;
        case WashState::FILL_PREWASH:
            return WashState::PREWASH;
        case WashState::PREWASH:
            return WashState::DRAIN_WASH;
            
        case WashState::DRAIN_WASH:
            return WashState::FILL_WASH;
        case WashState::FILL_WASH:
            return WashState::WASH;
        case WashState::WASH:
            return WashState::DRAIN_RINSE1;
            
        case WashState::DRAIN_RINSE1:
            return WashState::FILL_RINSE1;
        case WashState::FILL_RINSE1:
            return WashState::RINSE1;
        case WashState::RINSE1:
            return WashState::DRAIN_RINSE2;
            
        case WashState::DRAIN_RINSE2:
            return WashState::FILL_RINSE2;
        case WashState::FILL_RINSE2:
            return WashState::RINSE2;
        case WashState::RINSE2:
            return WashState::FINAL_DRAIN;
            
        case WashState::FINAL_DRAIN:
            return WashState::COMPLETE;
            
        case WashState::COMPLETE:
            return WashState::IDLE;  // Auto-transition to IDLE after complete
            
        default:
            return WashState::ERROR;
    }
}

unsigned long WashCycleController::getStateTimeout(WashState state) {
    switch (state) {
        case WashState::DRAIN_PREWASH:
        case WashState::DRAIN_WASH:
        case WashState::DRAIN_RINSE1:
        case WashState::DRAIN_RINSE2:
        case WashState::FINAL_DRAIN:
            return timingConfig.tpomp;
            
        case WashState::FILL_PREWASH:
        case WashState::FILL_WASH:
        case WashState::FILL_RINSE1:
        case WashState::FILL_RINSE2:
            return timingConfig.water_in_timer;
            
        case WashState::PREWASH:
            return timingConfig.washtime0;
            
        case WashState::WASH:
            return timingConfig.washtime1;
            
        case WashState::RINSE1:
            return timingConfig.washtime2;
            
        case WashState::RINSE2:
            return timingConfig.washtime3;
            
        default:
            return 60000; // Default 1 minute
    }
}
