/**
 * StateManager.h
 * 
 * Manages wash cycle state machine and transitions.
 * System always starts in IDLE state (no state recovery after power loss).
 */

#ifndef STATE_MANAGER_H
#define STATE_MANAGER_H

#include <Arduino.h>
#include <functional>

// Wash cycle states
enum class WashState : uint8_t {
    IDLE = 0,              // Waiting for user command
    DRAIN_PREWASH = 2,     // Drain before pre-wash
    FILL_PREWASH = 3,      // Fill water for pre-wash
    PREWASH = 4,           // Pre-wash/soak phase
    DRAIN_WASH = 5,        // Drain before main wash
    FILL_WASH = 6,         // Fill water for main wash
    WASH = 7,              // Main wash phase
    DRAIN_RINSE1 = 8,      // Drain before first rinse
    FILL_RINSE1 = 9,       // Fill water for first rinse
    RINSE1 = 10,           // First rinse phase
    DRAIN_RINSE2 = 11,     // Drain before second rinse
    FILL_RINSE2 = 12,      // Fill water for second rinse
    RINSE2 = 13,           // Second rinse phase
    FINAL_DRAIN = 14,      // Final drain
    COMPLETE = 15,         // Cycle complete
    ERROR = 255            // Error state
};

class StateManager {
public:
    // State change callback type
    typedef std::function<void(WashState oldState, WashState newState)> StateChangeCallback;
    
    StateManager();
    ~StateManager();
    
    // Initialization
    bool begin();
    
    // State control
    WashState getCurrentState();
    void setState(WashState newState);
    bool isValidTransition(WashState from, WashState to);
    
    // Cycle control
    bool startCycle();
    bool stopCycle();
    bool pauseCycle();
    bool resumeCycle();
    
    // State information
    String getStateDescription();
    String getStateDescription(WashState state);
    uint8_t getProgressPercent();
    unsigned long getElapsedTime();
    unsigned long getEstimatedRemaining();
    
    // State queries
    bool isIdle();
    bool isRunning();
    bool isPaused();
    bool isComplete();
    bool isError();
    bool isCriticalState(); // For OTA blocking
    
    // Callbacks
    void onStateChange(StateChangeCallback callback);
    
    // Timing
    void setCycleStartTime(unsigned long startTime);
    unsigned long getCycleStartTime();
    
private:
    WashState currentState;
    WashState previousState;
    bool paused;
    unsigned long cycleStartTime;
    unsigned long stateStartTime;
    StateChangeCallback stateChangeCallback;
    
    // Helper methods
    void notifyStateChange(WashState oldState, WashState newState);
    uint8_t calculateProgress();
    unsigned long estimateRemainingTime();
};

#endif // STATE_MANAGER_H
