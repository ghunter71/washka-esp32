/**
 * WaterControl.h
 * 
 * Controls water filling using gerkon (reed switch) sensor with debouncing.
 */

#ifndef WATER_CONTROL_H
#define WATER_CONTROL_H

#include <Arduino.h>
#include <functional>

class WaterControl {
public:
    // Fill result enumeration
    enum class FillResult {
        SUCCESS_GERKON,      // Threshold reached
        SUCCESS_TIMEOUT,     // Timeout reached (warning)
        ERROR_TIMEOUT,       // Timeout without reaching threshold
        ERROR_SENSOR         // Sensor malfunction detected
    };
    
    // Gerkon trigger callback type
    typedef std::function<void(uint16_t count)> GerkonTriggerCallback;
    
    WaterControl();
    ~WaterControl();
    
    // Initialization
    bool begin(uint8_t gerkonPin);
    
    // Water filling control
    FillResult fillWater(uint16_t gerkonThreshold, unsigned long timeoutMs);
    
    // Gerkon counter
    uint16_t getGerkonCount();
    void resetGerkonCount();
    
    // Debouncing
    void setDebounceMs(uint16_t ms);
    uint16_t getDebounceMs();
    
    // Callbacks
    void onGerkonTrigger(GerkonTriggerCallback callback);
    
    // Interrupt handler (must be public for ISR)
    void handleGerkonInterrupt();
    
private:
    uint8_t gerkonPin;
    volatile uint16_t gerkonCount;
    uint16_t debounceMs;
    volatile unsigned long lastTriggerTime;
    bool initialized;
    GerkonTriggerCallback triggerCallback;
    
    // Helper methods
    bool isGerkonTriggered();
};

#endif // WATER_CONTROL_H
