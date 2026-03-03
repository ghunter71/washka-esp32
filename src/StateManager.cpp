/**
 * StateManager.cpp
 * 
 * Implementation of wash cycle state machine.
 * Thread-safe with FreeRTOS mutex protection.
 * Async callback queue for non-blocking notifications.
 * 
 * @version 2.0.0
 */

#include "StateManager.h"
#include <Preferences.h>

// NVS keys for state persistence
#define NVS_STATE_NAMESPACE "washka_state"
#define NVS_KEY_CURRENT_STATE "curr_state"
#define NVS_KEY_PAUSED "paused"
#define NVS_KEY_CYCLE_START "cycle_start"
#define NVS_KEY_HAS_STORED "has_stored"

StateManager::StateManager() :
    currentState(WashState::IDLE),
    previousState(WashState::IDLE),
    paused(false),
    cycleStartTime(0),
    stateStartTime(0),
    stateChangeCallback(nullptr),
    stateMutex(nullptr),
    eventQueue(nullptr) {
}

StateManager::~StateManager() {
    if (stateMutex) {
        vSemaphoreDelete(stateMutex);
    }
    if (eventQueue) {
        vQueueDelete(eventQueue);
    }
}

bool StateManager::begin() {
    // Create mutex for thread safety
    stateMutex = xSemaphoreCreateMutex();
    if (stateMutex == nullptr) {
        Serial.println("ERROR: Failed to create StateManager mutex!");
        return false;
    }
    
    // Create event queue for async callbacks
    eventQueue = xQueueCreate(EVENT_QUEUE_SIZE, sizeof(StateChangeEvent));
    if (eventQueue == nullptr) {
        Serial.println("ERROR: Failed to create StateManager event queue!");
        vSemaphoreDelete(stateMutex);
        return false;
    }
    
    // Always start in IDLE state (no state recovery by default)
    // Call loadStateFromNVS() explicitly if recovery is needed
    currentState = WashState::IDLE;
    previousState = WashState::IDLE;
    paused = false;
    cycleStartTime = 0;
    stateStartTime = millis();
    
    Serial.println("✓ StateManager initialized (thread-safe, async callbacks)");
    return true;
}

// ============================================================================
// Mutex Helpers
// ============================================================================

bool StateManager::takeMutex(unsigned long timeoutMs) {
    if (stateMutex == nullptr) return false;
    return xSemaphoreTake(stateMutex, pdMS_TO_TICKS(timeoutMs)) == pdTRUE;
}

void StateManager::giveMutex() {
    if (stateMutex) {
        xSemaphoreGive(stateMutex);
    }
}

// ============================================================================
// State Control (Thread-Safe)
// ============================================================================

WashState StateManager::getCurrentState() {
    WashState state;
    if (takeMutex()) {
        state = currentState;
        giveMutex();
    } else {
        state = WashState::ERROR; // Safe default on mutex failure
    }
    return state;
}

bool StateManager::setState(WashState newState) {
    if (!takeMutex(200)) {
        Serial.println("ERROR: StateManager setState mutex timeout!");
        return false;
    }
    
    if (currentState == newState) {
        giveMutex();
        return true; // No change needed
    }
    
    // Validate transition
    if (!isValidTransition(currentState, newState)) {
        Serial.printf("WARNING: Invalid state transition: %s -> %s (allowed anyway for safety)\n",
                      getStateDescription(currentState).c_str(),
                      getStateDescription(newState).c_str());
        // Allow anyway for safety-critical transitions (ERROR, IDLE)
        if (newState != WashState::IDLE && newState != WashState::ERROR) {
            giveMutex();
            return false;
        }
    }
    
    WashState oldState = currentState;
    previousState = currentState;
    currentState = newState;
    stateStartTime = millis();
    
    Serial.printf("State transition: %s -> %s\n", 
                  getStateDescription(oldState).c_str(),
                  getStateDescription(newState).c_str());
    
    // Queue event for async callback (don't block here!)
    queueStateChangeEvent(oldState, newState);
    
    giveMutex();
    return true;
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
    
    // Paused state can resume
    if (from == to && paused) {
        return true;
    }
    
    return false;
}

// ============================================================================
// Cycle Control (Thread-Safe)
// ============================================================================

bool StateManager::startCycle() {
    if (!takeMutex(100)) {
        return false;
    }
    
    if (currentState != WashState::IDLE) {
        Serial.println("ERROR: Cannot start cycle - not in IDLE state");
        giveMutex();
        return false;
    }
    
    cycleStartTime = millis();
    WashState oldState = currentState;
    currentState = WashState::DRAIN_PREWASH;
    stateStartTime = millis();
    paused = false;
    
    Serial.println("✓ Wash cycle started");
    
    // Queue event
    StateChangeEvent event = {oldState, currentState, millis()};
    xQueueSend(eventQueue, &event, 0);
    
    giveMutex();
    return true;
}

bool StateManager::stopCycle() {
    if (!takeMutex(100)) {
        return false;
    }
    
    if (currentState == WashState::IDLE) {
        Serial.println("Already in IDLE state");
        giveMutex();
        return true;
    }
    
    WashState oldState = currentState;
    previousState = currentState;
    currentState = WashState::IDLE;
    paused = false;
    cycleStartTime = 0;
    
    Serial.printf("✓ Wash cycle stopped from state: %s\n", 
                  getStateDescription(oldState).c_str());
    
    // Queue event
    StateChangeEvent event = {oldState, WashState::IDLE, millis()};
    xQueueSend(eventQueue, &event, 0);
    
    giveMutex();
    return true;
}

bool StateManager::pauseCycle() {
    if (!takeMutex(100)) {
        return false;
    }
    
    if (currentState == WashState::IDLE || currentState == WashState::COMPLETE) {
        Serial.println("ERROR: Cannot pause - cycle not running");
        giveMutex();
        return false;
    }
    
    if (paused) {
        Serial.println("Already paused");
        giveMutex();
        return true;
    }
    
    paused = true;
    Serial.println("✓ Wash cycle paused");
    
    giveMutex();
    return true;
}

bool StateManager::resumeCycle() {
    if (!takeMutex(100)) {
        return false;
    }
    
    if (!paused) {
        Serial.println("ERROR: Cannot resume - not paused");
        giveMutex();
        return false;
    }
    
    paused = false;
    Serial.println("✓ Wash cycle resumed");
    
    giveMutex();
    return true;
}

// ============================================================================
// State Information
// ============================================================================

String StateManager::getStateDescription() {
    return getStateDescription(getCurrentState());
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
    unsigned long elapsed = 0;
    if (takeMutex()) {
        if (cycleStartTime != 0) {
            elapsed = millis() - cycleStartTime;
        }
        giveMutex();
    }
    return elapsed;
}

unsigned long StateManager::getEstimatedRemaining() {
    return estimateRemainingTime();
}

// ============================================================================
// State Queries (Thread-Safe)
// ============================================================================

bool StateManager::isIdle() {
    bool result = false;
    if (takeMutex()) {
        result = (currentState == WashState::IDLE);
        giveMutex();
    }
    return result;
}

bool StateManager::isRunning() {
    bool result = false;
    if (takeMutex()) {
        result = (currentState != WashState::IDLE && 
                  currentState != WashState::COMPLETE && 
                  currentState != WashState::ERROR &&
                  !paused);
        giveMutex();
    }
    return result;
}

bool StateManager::isPaused() {
    bool result = false;
    if (takeMutex()) {
        result = paused;
        giveMutex();
    }
    return result;
}

bool StateManager::isComplete() {
    bool result = false;
    if (takeMutex()) {
        result = (currentState == WashState::COMPLETE);
        giveMutex();
    }
    return result;
}

bool StateManager::isError() {
    bool result = false;
    if (takeMutex()) {
        result = (currentState == WashState::ERROR);
        giveMutex();
    }
    return result;
}

bool StateManager::isCriticalState() {
    bool result = false;
    if (takeMutex()) {
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
                result = true;
                break;
            default:
                result = false;
        }
        giveMutex();
    }
    return result;
}

// ============================================================================
// Callbacks
// ============================================================================

void StateManager::onStateChange(StateChangeCallback callback) {
    if (takeMutex()) {
        stateChangeCallback = callback;
        giveMutex();
    }
}

void StateManager::queueStateChangeEvent(WashState oldState, WashState newState) {
    // Called from within mutex-protected section
    StateChangeEvent event = {oldState, newState, millis()};
    
    // Try to send without blocking (we're in a critical section)
    if (xQueueSend(eventQueue, &event, 0) != pdTRUE) {
        Serial.println("WARNING: State event queue full, dropping event");
    }
}

void StateManager::processCallbacks() {
    // Process all pending events from the queue
    StateChangeEvent event;
    int processedCount = 0;
    
    while (xQueueReceive(eventQueue, &event, 0) == pdTRUE && processedCount < 5) {
        // Call callback outside of mutex lock
        if (stateChangeCallback) {
            stateChangeCallback(event.oldState, event.newState);
        }
        processedCount++;
    }
}

void StateManager::notifyStateChange(WashState oldState, WashState newState) {
    // Legacy synchronous method - use processCallbacks() instead
    if (stateChangeCallback) {
        stateChangeCallback(oldState, newState);
    }
}

// ============================================================================
// State Persistence for Recovery
// ============================================================================

bool StateManager::saveStateToNVS() {
    Preferences prefs;
    if (!prefs.begin(NVS_STATE_NAMESPACE, false)) {
        return false;
    }
    
    if (takeMutex()) {
        prefs.putUChar(NVS_KEY_CURRENT_STATE, static_cast<uint8_t>(currentState));
        prefs.putBool(NVS_KEY_PAUSED, paused);
        prefs.putULong(NVS_KEY_CYCLE_START, cycleStartTime);
        prefs.putBool(NVS_KEY_HAS_STORED, true);
        giveMutex();
    }
    
    prefs.end();
    Serial.println("✓ State saved to NVS");
    return true;
}

bool StateManager::loadStateFromNVS() {
    Preferences prefs;
    if (!prefs.begin(NVS_STATE_NAMESPACE, true)) {
        return false;
    }
    
    bool hasStored = prefs.getBool(NVS_KEY_HAS_STORED, false);
    if (!hasStored) {
        prefs.end();
        return false;
    }
    
    if (takeMutex()) {
        currentState = static_cast<WashState>(prefs.getUChar(NVS_KEY_CURRENT_STATE, 0));
        paused = prefs.getBool(NVS_KEY_PAUSED, false);
        cycleStartTime = prefs.getULong(NVS_KEY_CYCLE_START, 0);
        
        // Validate loaded state
        if (currentState == WashState::ERROR || currentState == WashState::IDLE) {
            // Safe states, nothing to recover
            giveMutex();
            prefs.end();
            return false;
        }
        
        stateStartTime = millis();
        giveMutex();
    }
    
    prefs.end();
    Serial.printf("✓ State loaded from NVS: %s\n", getStateDescription().c_str());
    return true;
}

bool StateManager::hasStoredState() {
    Preferences prefs;
    if (!prefs.begin(NVS_STATE_NAMESPACE, true)) {
        return false;
    }
    bool hasStored = prefs.getBool(NVS_KEY_HAS_STORED, false);
    prefs.end();
    return hasStored;
}

void StateManager::clearStoredState() {
    Preferences prefs;
    prefs.begin(NVS_STATE_NAMESPACE, false);
    prefs.clear();
    prefs.end();
    Serial.println("✓ Stored state cleared");
}

// ============================================================================
// Timing
// ============================================================================

void StateManager::setCycleStartTime(unsigned long startTime) {
    if (takeMutex()) {
        cycleStartTime = startTime;
        giveMutex();
    }
}

unsigned long StateManager::getCycleStartTime() {
    unsigned long startTime = 0;
    if (takeMutex()) {
        startTime = cycleStartTime;
        giveMutex();
    }
    return startTime;
}

// ============================================================================
// Private Helper Methods
// ============================================================================

uint8_t StateManager::calculateProgress() {
    uint8_t progress = 0;
    if (takeMutex()) {
        if (currentState == WashState::IDLE) {
            progress = 0;
        } else if (currentState == WashState::COMPLETE) {
            progress = 100;
        } else {
            // Approximate progress based on state
            // Total states in cycle: DRAIN_PREWASH(2) to COMPLETE(15) = 14 states
            uint8_t stateNum = static_cast<uint8_t>(currentState);
            if (stateNum >= 2 && stateNum <= 15) {
                progress = ((stateNum - 2) * 100) / 13;
            }
        }
        giveMutex();
    }
    return progress;
}

unsigned long StateManager::estimateRemainingTime() {
    WashState state = getCurrentState();
    if (state == WashState::IDLE || state == WashState::COMPLETE) {
        return 0;
    }
    
    // Rough estimate based on typical cycle times
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
