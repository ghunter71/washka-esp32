/**
 * Washka ESP32 - Dishwasher Control System
 * 
 * Main entry point for the ESP32-based dishwasher control system.
 * Uses ApplicationKernel for dependency injection and lifecycle management.
 * 
 * @version 2.0.0
 * @author Washka Team
 */

#include <Arduino.h>
#include <esp_task_wdt.h>
#include "ApplicationKernel.h"

// Version information
const char* FIRMWARE_VERSION = "2.0.0";
const char* BUILD_DATE = __DATE__;
const char* BUILD_TIME = __TIME__;

/**
 * Setup function - runs once at startup
 */
void setup() {
    // Initialize serial communication
    Serial.begin(115200);
    delay(2000);  // Wait for serial to stabilize
    
    Serial.println("\n\n");
    Serial.println("========================================");
    Serial.println("  Washka ESP32 - Dishwasher Control");
    Serial.println("========================================");
    Serial.printf("  Version: %s\n", FIRMWARE_VERSION);
    Serial.printf("  Build: %s %s\n", BUILD_DATE, BUILD_TIME);
    Serial.println("========================================\n");
    
    // Configure watchdog timer (60 seconds for SSL operations)
    esp_task_wdt_init(60, true);
    esp_task_wdt_add(NULL);
    Serial.println("✓ Watchdog timer configured (60s timeout)");
    
    // Get application kernel instance
    ApplicationKernel& kernel = ApplicationKernel::getInstance();
    
    // Set up error callback
    kernel.onError([](const ApplicationKernel::ErrorInfo& error) {
        Serial.printf("SYSTEM ERROR [%s]: %s\n", 
                      error.component.c_str(), 
                      error.message.c_str());
    });
    
    // Set up state change callback
    kernel.onStateChange([](ApplicationKernel::AppState oldState, 
                            ApplicationKernel::AppState newState) {
        Serial.printf("Kernel state: %d -> %d\n", oldState, newState);
    });
    
    // Initialize the system
    if (!kernel.initialize()) {
        Serial.println("\n✗✗✗ CRITICAL INITIALIZATION FAILURE ✗✗✗");
        Serial.println("System will enter safe mode...");
        kernel.enterSafeMode();
    }
    
    // Print system status
    Serial.println("\nSystem Status:");
    if (kernel.getWiFiManager()) {
        Serial.printf("  WiFi: %s\n", 
                      kernel.getWiFiManager()->getStatus() == WiFiManager::ConnectionStatus::CONNECTED 
                      ? "Connected" : "AP Mode");
        Serial.printf("  IP: %s\n", kernel.getWiFiManager()->getIPAddress().c_str());
    }
    if (kernel.getStateManager()) {
        Serial.printf("  State: %s\n", 
                      kernel.getStateManager()->getStateDescription().c_str());
    }
    Serial.println();
}

/**
 * Main loop - runs continuously
 */
void loop() {
    // Get kernel instance and run
    ApplicationKernel& kernel = ApplicationKernel::getInstance();
    
    // Run kernel main loop
    kernel.run();
    
    // Small delay to prevent task starvation
    delay(10);
}
