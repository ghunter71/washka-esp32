/**
 * PinLegend.h
 * 
 * WROOM-32 Pin Legend and Validation
 * Provides comprehensive pin information for ESP32 WROOM-32 module
 */

#ifndef PIN_LEGEND_H
#define PIN_LEGEND_H

#include <Arduino.h>
#include <vector>

/**
 * Pin capability flags
 */
enum PinCapability {
    CAP_DIGITAL_IO = 0x01,
    CAP_ADC1 = 0x02,
    CAP_ADC2 = 0x04,
    CAP_DAC = 0x08,
    CAP_TOUCH = 0x10,
    CAP_RTC = 0x20,
    CAP_INPUT_ONLY = 0x40
};

/**
 * Pin restriction flags
 */
enum PinRestriction {
    RESTRICT_NONE = 0x00,
    RESTRICT_STRAPPING = 0x01,
    RESTRICT_FLASH = 0x02,
    RESTRICT_SERIAL = 0x04,
    RESTRICT_INPUT_ONLY = 0x08,
    RESTRICT_ADC2_WIFI = 0x10
};

/**
 * Pin information structure
 */
struct PinInfo {
    uint8_t gpio;
    const char* label;
    const char* description;
    uint8_t capabilities;
    uint8_t restrictions;
    const char* warning;
    bool recommended;
    
    // Helper methods
    bool hasCapability(PinCapability cap) const {
        return (capabilities & cap) != 0;
    }
    
    bool hasRestriction(PinRestriction res) const {
        return (restrictions & res) != 0;
    }
    
    bool isInputOnly() const {
        return hasCapability(CAP_INPUT_ONLY);
    }
    
    bool isStrapping() const {
        return hasRestriction(RESTRICT_STRAPPING);
    }
    
    bool isFlashPin() const {
        return hasRestriction(RESTRICT_FLASH);
    }
    
    bool canUseForOutput() const {
        return !isInputOnly() && !isFlashPin() && recommended;
    }
};

/**
 * Complete WROOM-32 pin map
 */
class PinLegend {
public:
    static const PinInfo* getPinInfo(uint8_t gpio);
    static bool isPinValid(uint8_t gpio);
    static bool isPinRecommended(uint8_t gpio);
    static const char* getPinWarning(uint8_t gpio);
    static String getPinDescription(uint8_t gpio);
    static std::vector<uint8_t> getRecommendedPins();
    static String getPinLegendJson();
    
private:
    static const PinInfo PIN_MAP[];
    static const size_t PIN_MAP_SIZE;
};

#endif // PIN_LEGEND_H
