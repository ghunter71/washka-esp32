/**
 * PinLegend.cpp
 * 
 * Implementation of WROOM-32 pin legend and validation
 */

#include "PinLegend.h"
#include <ArduinoJson.h>

// Complete WROOM-32 pin map with detailed information
const PinInfo PinLegend::PIN_MAP[] = {
    // GPIO 0 - Strapping pin (boot mode)
    {0, "D0", "GPIO0/BOOT", CAP_DIGITAL_IO | CAP_RTC | CAP_TOUCH, 
     RESTRICT_STRAPPING, "Strapping pin - controls boot mode. Must be HIGH during boot.", false},
    
    // GPIO 1 - TX (Serial)
    {1, "TX", "GPIO1/U0TXD", CAP_DIGITAL_IO, 
     RESTRICT_SERIAL, "Serial TX - avoid using if Serial is needed", false},
    
    // GPIO 2 - Strapping pin (boot mode) + onboard LED
    {2, "D2", "GPIO2/LED", CAP_DIGITAL_IO | CAP_RTC | CAP_TOUCH, 
     RESTRICT_STRAPPING, "Strapping pin - must be LOW during boot. Often connected to onboard LED.", false},
    
    // GPIO 3 - RX (Serial)
    {3, "RX", "GPIO3/U0RXD", CAP_DIGITAL_IO, 
     RESTRICT_SERIAL, "Serial RX - avoid using if Serial is needed", false},
    
    // GPIO 4 - Safe to use
    {4, "D4", "GPIO4/RTC", CAP_DIGITAL_IO | CAP_RTC | CAP_TOUCH, 
     RESTRICT_NONE, "", true},
    
    // GPIO 5 - Strapping pin (SDIO timing)
    {5, "D5", "GPIO5", CAP_DIGITAL_IO, 
     RESTRICT_STRAPPING, "Strapping pin - SDIO timing. Generally safe but use with caution.", true},
    
    // GPIO 6-11 - Flash pins (DO NOT USE)
    {6, "SCK", "GPIO6/FLASH_SCK", CAP_DIGITAL_IO, 
     RESTRICT_FLASH, "Connected to SPI flash - DO NOT USE", false},
    {7, "SD0", "GPIO7/FLASH_D0", CAP_DIGITAL_IO, 
     RESTRICT_FLASH, "Connected to SPI flash - DO NOT USE", false},
    {8, "SD1", "GPIO8/FLASH_D1", CAP_DIGITAL_IO, 
     RESTRICT_FLASH, "Connected to SPI flash - DO NOT USE", false},
    {9, "SD2", "GPIO9/FLASH_D2", CAP_DIGITAL_IO, 
     RESTRICT_FLASH, "Connected to SPI flash - DO NOT USE", false},
    {10, "SD3", "GPIO10/FLASH_D3", CAP_DIGITAL_IO, 
     RESTRICT_FLASH, "Connected to SPI flash - DO NOT USE", false},
    {11, "CMD", "GPIO11/FLASH_CMD", CAP_DIGITAL_IO, 
     RESTRICT_FLASH, "Connected to SPI flash - DO NOT USE", false},
    
    // GPIO 12 - Strapping pin (flash voltage)
    {12, "D12", "GPIO12/ADC2_5", CAP_DIGITAL_IO | CAP_ADC2 | CAP_RTC | CAP_TOUCH, 
     RESTRICT_STRAPPING, "Strapping pin - flash voltage. Boot fails if pulled HIGH.", false},
    
    // GPIO 13 - Safe to use
    {13, "D13", "GPIO13/ADC2_4", CAP_DIGITAL_IO | CAP_ADC2 | CAP_RTC | CAP_TOUCH, 
     RESTRICT_ADC2_WIFI, "ADC2 - cannot use when WiFi is active", true},
    
    // GPIO 14 - Safe to use
    {14, "D14", "GPIO14/ADC2_6", CAP_DIGITAL_IO | CAP_ADC2 | CAP_RTC | CAP_TOUCH, 
     RESTRICT_ADC2_WIFI, "ADC2 - cannot use when WiFi is active", true},
    
    // GPIO 15 - Strapping pin (boot messages)
    {15, "D15", "GPIO15/ADC2_3", CAP_DIGITAL_IO | CAP_ADC2 | CAP_RTC | CAP_TOUCH, 
     RESTRICT_STRAPPING, "Strapping pin - boot messages. Boot fails if pulled HIGH.", false},
    
    // GPIO 16-19 - Safe to use
    {16, "D16", "GPIO16", CAP_DIGITAL_IO, 
     RESTRICT_NONE, "", true},
    {17, "D17", "GPIO17", CAP_DIGITAL_IO, 
     RESTRICT_NONE, "", true},
    {18, "D18", "GPIO18", CAP_DIGITAL_IO, 
     RESTRICT_NONE, "", true},
    {19, "D19", "GPIO19", CAP_DIGITAL_IO, 
     RESTRICT_NONE, "", true},
    
    // GPIO 21-23 - Safe to use
    {21, "D21", "GPIO21/I2C_SDA", CAP_DIGITAL_IO, 
     RESTRICT_NONE, "", true},
    {22, "D22", "GPIO22/I2C_SCL", CAP_DIGITAL_IO, 
     RESTRICT_NONE, "", true},
    {23, "D23", "GPIO23", CAP_DIGITAL_IO, 
     RESTRICT_NONE, "", true},
    
    // GPIO 25-27 - Safe to use (DAC capable)
    {25, "D25", "GPIO25/ADC2_8/DAC1", CAP_DIGITAL_IO | CAP_ADC2 | CAP_DAC | CAP_RTC, 
     RESTRICT_ADC2_WIFI, "ADC2/DAC1 - ADC2 cannot use when WiFi is active", true},
    {26, "D26", "GPIO26/ADC2_9/DAC2", CAP_DIGITAL_IO | CAP_ADC2 | CAP_DAC | CAP_RTC, 
     RESTRICT_ADC2_WIFI, "ADC2/DAC2 - ADC2 cannot use when WiFi is active", true},
    {27, "D27", "GPIO27/ADC2_7", CAP_DIGITAL_IO | CAP_ADC2 | CAP_RTC | CAP_TOUCH, 
     RESTRICT_ADC2_WIFI, "ADC2 - cannot use when WiFi is active", true},
    
    // GPIO 32-33 - Safe to use (ADC1)
    {32, "D32", "GPIO32/ADC1_4", CAP_DIGITAL_IO | CAP_ADC1 | CAP_RTC | CAP_TOUCH, 
     RESTRICT_NONE, "", true},
    {33, "D33", "GPIO33/ADC1_5", CAP_DIGITAL_IO | CAP_ADC1 | CAP_RTC | CAP_TOUCH, 
     RESTRICT_NONE, "", true},
    
    // GPIO 34-39 - Input only (no pull-up/down)
    {34, "D34", "GPIO34/ADC1_6", CAP_INPUT_ONLY | CAP_ADC1 | CAP_RTC, 
     RESTRICT_INPUT_ONLY, "Input only - no internal pull-up/pull-down resistors", true},
    {35, "D35", "GPIO35/ADC1_7", CAP_INPUT_ONLY | CAP_ADC1 | CAP_RTC, 
     RESTRICT_INPUT_ONLY, "Input only - no internal pull-up/pull-down resistors", true},
    {36, "VP/D36", "GPIO36/ADC1_0/VP", CAP_INPUT_ONLY | CAP_ADC1 | CAP_RTC, 
     RESTRICT_INPUT_ONLY, "Input only - no internal pull-up/pull-down resistors", true},
    {37, "D37", "GPIO37/ADC1_1", CAP_INPUT_ONLY | CAP_ADC1 | CAP_RTC, 
     RESTRICT_INPUT_ONLY, "Input only - not available on most modules", false},
    {38, "D38", "GPIO38/ADC1_2", CAP_INPUT_ONLY | CAP_ADC1 | CAP_RTC, 
     RESTRICT_INPUT_ONLY, "Input only - not available on most modules", false},
    {39, "VN/D39", "GPIO39/ADC1_3/VN", CAP_INPUT_ONLY | CAP_ADC1 | CAP_RTC, 
     RESTRICT_INPUT_ONLY, "Input only - no internal pull-up/pull-down resistors", true}
};

const size_t PinLegend::PIN_MAP_SIZE = sizeof(PIN_MAP) / sizeof(PIN_MAP[0]);

const PinInfo* PinLegend::getPinInfo(uint8_t gpio) {
    for (size_t i = 0; i < PIN_MAP_SIZE; i++) {
        if (PIN_MAP[i].gpio == gpio) {
            return &PIN_MAP[i];
        }
    }
    return nullptr;
}

bool PinLegend::isPinValid(uint8_t gpio) {
    const PinInfo* info = getPinInfo(gpio);
    if (!info) return false;
    
    // Flash pins are never valid
    if (info->isFlashPin()) return false;
    
    // Pin must be in valid range
    return gpio <= 39;
}

bool PinLegend::isPinRecommended(uint8_t gpio) {
    const PinInfo* info = getPinInfo(gpio);
    return info && info->recommended;
}

const char* PinLegend::getPinWarning(uint8_t gpio) {
    const PinInfo* info = getPinInfo(gpio);
    return info ? info->warning : "Unknown pin";
}

String PinLegend::getPinDescription(uint8_t gpio) {
    const PinInfo* info = getPinInfo(gpio);
    if (!info) return "Unknown";
    
    String desc = String(info->description);
    
    // Add capability information
    if (info->hasCapability(CAP_ADC1)) desc += " [ADC1]";
    if (info->hasCapability(CAP_ADC2)) desc += " [ADC2]";
    if (info->hasCapability(CAP_DAC)) desc += " [DAC]";
    if (info->hasCapability(CAP_TOUCH)) desc += " [Touch]";
    if (info->hasCapability(CAP_RTC)) desc += " [RTC]";
    if (info->isInputOnly()) desc += " [Input Only]";
    
    return desc;
}

std::vector<uint8_t> PinLegend::getRecommendedPins() {
    std::vector<uint8_t> pins;
    for (size_t i = 0; i < PIN_MAP_SIZE; i++) {
        if (PIN_MAP[i].recommended && !PIN_MAP[i].isInputOnly()) {
            pins.push_back(PIN_MAP[i].gpio);
        }
    }
    return pins;
}

String PinLegend::getPinLegendJson() {
    DynamicJsonDocument doc(8192);
    JsonArray pinsArray = doc.createNestedArray("pins");
    
    for (size_t i = 0; i < PIN_MAP_SIZE; i++) {
        const PinInfo& pin = PIN_MAP[i];
        JsonObject pinObj = pinsArray.createNestedObject();
        
        pinObj["gpio"] = pin.gpio;
        pinObj["label"] = pin.label;
        pinObj["description"] = pin.description;
        pinObj["recommended"] = pin.recommended;
        pinObj["warning"] = pin.warning;
        
        // Capabilities
        JsonArray caps = pinObj.createNestedArray("capabilities");
        if (pin.hasCapability(CAP_DIGITAL_IO)) caps.add("Digital I/O");
        if (pin.hasCapability(CAP_ADC1)) caps.add("ADC1");
        if (pin.hasCapability(CAP_ADC2)) caps.add("ADC2");
        if (pin.hasCapability(CAP_DAC)) caps.add("DAC");
        if (pin.hasCapability(CAP_TOUCH)) caps.add("Touch");
        if (pin.hasCapability(CAP_RTC)) caps.add("RTC");
        if (pin.hasCapability(CAP_INPUT_ONLY)) caps.add("Input Only");
        
        // Restrictions
        JsonArray restrictions = pinObj.createNestedArray("restrictions");
        if (pin.hasRestriction(RESTRICT_STRAPPING)) restrictions.add("Strapping");
        if (pin.hasRestriction(RESTRICT_FLASH)) restrictions.add("Flash");
        if (pin.hasRestriction(RESTRICT_SERIAL)) restrictions.add("Serial");
        if (pin.hasRestriction(RESTRICT_INPUT_ONLY)) restrictions.add("Input Only");
        if (pin.hasRestriction(RESTRICT_ADC2_WIFI)) restrictions.add("ADC2/WiFi");
        
        pinObj["canUseForOutput"] = pin.canUseForOutput();
        pinObj["isInputOnly"] = pin.isInputOnly();
        pinObj["isStrapping"] = pin.isStrapping();
        pinObj["isFlash"] = pin.isFlashPin();
    }
    
    String output;
    serializeJson(doc, output);
    return output;
}
