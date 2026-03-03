/**
 * ActuatorManager.h
 * 
 * Manages hardware actuators with safety checks and mutual exclusion.
 */

#ifndef ACTUATOR_MANAGER_H
#define ACTUATOR_MANAGER_H

#include <Arduino.h>
#include "ConfigManager.h"

class ActuatorManager {
public:
    // Actuator status structure
    struct ActuatorStatus {
        bool washengine;
        bool pompa;
        bool water_valve;
        bool powder;
        bool led;
        
        ActuatorStatus() : 
            washengine(false), 
            pompa(false), 
            water_valve(false), 
            powder(false), 
            led(false) {}
    };
    
    ActuatorManager();
    ~ActuatorManager();
    
    // Initialization
    bool begin(const ConfigManager::PinConfig& pins);
    
    // Individual actuator control
    bool setWashEngine(bool state);
    bool setPump(bool state);
    bool setWaterValve(bool state);
    bool setPowderDispenser(bool state);
    bool setLED(bool state);
    
    // Safety checks
    bool canFillWater();   // Check pump is off
    bool canDrain();       // Check valve is off
    
    // Emergency stop - immediately disable all outputs
    void emergencyStop();
    
    // Status
    ActuatorStatus getStatus();
    
private:
    ConfigManager::PinConfig pinConfig;
    ActuatorStatus currentStatus;
    bool initialized;
    
    // Helper methods
    void setActuatorState(uint8_t pin, bool state);
    bool getActuatorState(uint8_t pin);
};

#endif // ACTUATOR_MANAGER_H
