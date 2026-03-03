/**
 * WashCycleController.h
 * 
 * Controls automatic execution of wash cycle state machine.
 * Manages actuators, timers, and state transitions.
 */

#ifndef WASH_CYCLE_CONTROLLER_H
#define WASH_CYCLE_CONTROLLER_H

#include <Arduino.h>
#include "StateManager.h"
#include "ActuatorManager.h"
#include "WaterControl.h"
#include "ConfigManager.h"

class WashCycleController {
public:
    WashCycleController(StateManager* state, 
                       ActuatorManager* actuators,
                       WaterControl* water,
                       ConfigManager* config);
    ~WashCycleController();
    
    // Initialization
    bool begin();
    
    // Main loop - call this from main loop
    void loop();
    
private:
    // State execution
    void executeCurrentState();
    bool checkStateCompletion();
    void transitionToNextState();
    
    // Phase starters
    void startDrainPhase();
    void startFillPhase(bool addPowder);
    void startWashPhase();
    void stopAllActuators();
    
    // Completion checks
    bool isDrainComplete();
    bool isFillComplete();
    bool isWashComplete();
    
    // Get next state in sequence
    WashState getNextState(WashState current);
    
    // Get timeout for current state
    unsigned long getStateTimeout(WashState state);
    
    // Component references
    StateManager* stateManager;
    ActuatorManager* actuatorManager;
    WaterControl* waterControl;
    ConfigManager* configManager;
    
    // State tracking
    unsigned long stateStartTime;
    unsigned long stateTimeout;
    int gerkonStartCount;
    int gerkonThreshold;
    bool stateInitialized;
    
    // Powder dispenser tracking
    bool powderActive;
    unsigned long powderStartTime;
    const unsigned long POWDER_DURATION = 1000;  // 1 second
    
    // Timing configuration cache
    ConfigManager::TimingConfig timingConfig;
};

#endif // WASH_CYCLE_CONTROLLER_H
