/**
 * test_pin_legend.cpp
 * 
 * Unit tests for PinLegend functionality
 */

#ifdef UNIT_TEST

#include <unity.h>
#include "PinLegend.h"

void setUp(void) {
    // Set up before each test
}

void tearDown(void) {
    // Clean up after each test
}

// Test pin validation
void test_pin_validation() {
    // Valid pins
    TEST_ASSERT_TRUE(PinLegend::isPinValid(4));
    TEST_ASSERT_TRUE(PinLegend::isPinValid(32));
    TEST_ASSERT_TRUE(PinLegend::isPinValid(33));
    
    // Flash pins (invalid)
    TEST_ASSERT_FALSE(PinLegend::isPinValid(6));
    TEST_ASSERT_FALSE(PinLegend::isPinValid(7));
    TEST_ASSERT_FALSE(PinLegend::isPinValid(8));
    TEST_ASSERT_FALSE(PinLegend::isPinValid(9));
    TEST_ASSERT_FALSE(PinLegend::isPinValid(10));
    TEST_ASSERT_FALSE(PinLegend::isPinValid(11));
    
    // Out of range
    TEST_ASSERT_FALSE(PinLegend::isPinValid(40));
    TEST_ASSERT_FALSE(PinLegend::isPinValid(255));
}

// Test pin recommendations
void test_pin_recommendations() {
    // Recommended pins
    TEST_ASSERT_TRUE(PinLegend::isPinRecommended(4));
    TEST_ASSERT_TRUE(PinLegend::isPinRecommended(32));
    TEST_ASSERT_TRUE(PinLegend::isPinRecommended(33));
    
    // Not recommended (strapping pins)
    TEST_ASSERT_FALSE(PinLegend::isPinRecommended(0));
    TEST_ASSERT_FALSE(PinLegend::isPinRecommended(2));
    TEST_ASSERT_FALSE(PinLegend::isPinRecommended(12));
    TEST_ASSERT_FALSE(PinLegend::isPinRecommended(15));
    
    // Flash pins
    TEST_ASSERT_FALSE(PinLegend::isPinRecommended(6));
}

// Test pin warnings
void test_pin_warnings() {
    // Strapping pin should have warning
    const char* warning0 = PinLegend::getPinWarning(0);
    TEST_ASSERT_NOT_NULL(warning0);
    TEST_ASSERT_TRUE(strlen(warning0) > 0);
    
    // Flash pin should have warning
    const char* warning6 = PinLegend::getPinWarning(6);
    TEST_ASSERT_NOT_NULL(warning6);
    TEST_ASSERT_TRUE(strlen(warning6) > 0);
    
    // Safe pin should have no warning
    const char* warning32 = PinLegend::getPinWarning(32);
    TEST_ASSERT_NOT_NULL(warning32);
    TEST_ASSERT_EQUAL(0, strlen(warning32));
}

// Test pin info retrieval
void test_pin_info() {
    // Get info for GPIO 32
    const PinInfo* info = PinLegend::getPinInfo(32);
    TEST_ASSERT_NOT_NULL(info);
    TEST_ASSERT_EQUAL(32, info->gpio);
    TEST_ASSERT_TRUE(info->recommended);
    TEST_ASSERT_TRUE(info->canUseForOutput());
    TEST_ASSERT_FALSE(info->isInputOnly());
    TEST_ASSERT_FALSE(info->isStrapping());
    TEST_ASSERT_FALSE(info->isFlashPin());
    
    // Get info for input-only pin (GPIO 34)
    const PinInfo* info34 = PinLegend::getPinInfo(34);
    TEST_ASSERT_NOT_NULL(info34);
    TEST_ASSERT_TRUE(info34->isInputOnly());
    TEST_ASSERT_FALSE(info34->canUseForOutput());
    
    // Get info for strapping pin (GPIO 0)
    const PinInfo* info0 = PinLegend::getPinInfo(0);
    TEST_ASSERT_NOT_NULL(info0);
    TEST_ASSERT_TRUE(info0->isStrapping());
    TEST_ASSERT_FALSE(info0->recommended);
    
    // Get info for flash pin (GPIO 6)
    const PinInfo* info6 = PinLegend::getPinInfo(6);
    TEST_ASSERT_NOT_NULL(info6);
    TEST_ASSERT_TRUE(info6->isFlashPin());
    TEST_ASSERT_FALSE(info6->recommended);
}

// Test pin capabilities
void test_pin_capabilities() {
    // GPIO 32 has ADC1
    const PinInfo* info32 = PinLegend::getPinInfo(32);
    TEST_ASSERT_NOT_NULL(info32);
    TEST_ASSERT_TRUE(info32->hasCapability(CAP_ADC1));
    TEST_ASSERT_TRUE(info32->hasCapability(CAP_DIGITAL_IO));
    TEST_ASSERT_FALSE(info32->hasCapability(CAP_ADC2));
    
    // GPIO 25 has DAC
    const PinInfo* info25 = PinLegend::getPinInfo(25);
    TEST_ASSERT_NOT_NULL(info25);
    TEST_ASSERT_TRUE(info25->hasCapability(CAP_DAC));
    TEST_ASSERT_TRUE(info25->hasCapability(CAP_ADC2));
    
    // GPIO 34 is input only
    const PinInfo* info34 = PinLegend::getPinInfo(34);
    TEST_ASSERT_NOT_NULL(info34);
    TEST_ASSERT_TRUE(info34->hasCapability(CAP_INPUT_ONLY));
}

// Test recommended pins list
void test_recommended_pins_list() {
    std::vector<uint8_t> recommended = PinLegend::getRecommendedPins();
    
    // Should have multiple recommended pins
    TEST_ASSERT_TRUE(recommended.size() > 10);
    
    // Should include known safe pins
    bool has4 = false, has32 = false, has33 = false;
    for (uint8_t pin : recommended) {
        if (pin == 4) has4 = true;
        if (pin == 32) has32 = true;
        if (pin == 33) has33 = true;
    }
    TEST_ASSERT_TRUE(has4);
    TEST_ASSERT_TRUE(has32);
    TEST_ASSERT_TRUE(has33);
    
    // Should not include flash pins
    for (uint8_t pin : recommended) {
        TEST_ASSERT_FALSE(pin >= 6 && pin <= 11);
    }
    
    // Should not include input-only pins
    for (uint8_t pin : recommended) {
        const PinInfo* info = PinLegend::getPinInfo(pin);
        TEST_ASSERT_FALSE(info->isInputOnly());
    }
}

// Test JSON export
void test_json_export() {
    String json = PinLegend::getPinLegendJson();
    
    // Should not be empty
    TEST_ASSERT_TRUE(json.length() > 0);
    
    // Should contain expected keys
    TEST_ASSERT_TRUE(json.indexOf("\"pins\"") >= 0);
    TEST_ASSERT_TRUE(json.indexOf("\"gpio\"") >= 0);
    TEST_ASSERT_TRUE(json.indexOf("\"label\"") >= 0);
    TEST_ASSERT_TRUE(json.indexOf("\"description\"") >= 0);
    TEST_ASSERT_TRUE(json.indexOf("\"recommended\"") >= 0);
    TEST_ASSERT_TRUE(json.indexOf("\"capabilities\"") >= 0);
    TEST_ASSERT_TRUE(json.indexOf("\"restrictions\"") >= 0);
}

// Test pin description
void test_pin_description() {
    // GPIO 32 description
    String desc32 = PinLegend::getPinDescription(32);
    TEST_ASSERT_TRUE(desc32.length() > 0);
    TEST_ASSERT_TRUE(desc32.indexOf("GPIO32") >= 0);
    TEST_ASSERT_TRUE(desc32.indexOf("ADC1") >= 0);
    
    // GPIO 25 description (has DAC)
    String desc25 = PinLegend::getPinDescription(25);
    TEST_ASSERT_TRUE(desc25.indexOf("DAC") >= 0);
}

// Main test runner
int main(int argc, char **argv) {
    UNITY_BEGIN();
    
    RUN_TEST(test_pin_validation);
    RUN_TEST(test_pin_recommendations);
    RUN_TEST(test_pin_warnings);
    RUN_TEST(test_pin_info);
    RUN_TEST(test_pin_capabilities);
    RUN_TEST(test_recommended_pins_list);
    RUN_TEST(test_json_export);
    RUN_TEST(test_pin_description);
    
    return UNITY_END();
}

#endif // UNIT_TEST
