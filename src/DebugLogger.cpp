/**
 * DebugLogger.cpp
 * 
 * Implementation of centralized logging system.
 */

#include "DebugLogger.h"

// Initialize static instance
DebugLogger* DebugLogger::instance = nullptr;

/**
 * Constructor
 */
DebugLogger::DebugLogger() 
    : currentLogLevel(LogLevel::DEBUG),
      webServer(nullptr),
      webSerialEnabled(false),
      stateDumpCallback(nullptr) {
    instance = this;
}

/**
 * Destructor
 */
DebugLogger::~DebugLogger() {
    instance = nullptr;
}

/**
 * Get singleton instance
 */
DebugLogger& DebugLogger::getInstance() {
    static DebugLogger instance;
    return instance;
}

/**
 * Initialize the logger with WebSerial support
 */
bool DebugLogger::begin(AsyncWebServer* server) {
    if (server == nullptr) {
        Serial.println("ERROR: DebugLogger::begin() - server is null");
        return false;
    }
    
    webServer = server;
    
    // Initialize WebSerial
    WebSerial.begin(webServer);
    WebSerial.msgCallback(handleWebSerialMessage);
    webSerialEnabled = true;
    
    info("DebugLogger initialized");
    
    return true;
}

/**
 * Log a message with specified level
 */
void DebugLogger::log(LogLevel level, const String& message) {
    // Filter based on log level
    if (level < currentLogLevel) {
        return;
    }
    
    String formattedMessage = formatMessage(level, message);
    
    // Output to Serial
    outputToSerial(formattedMessage);
    
    // Output to WebSerial if enabled
    if (webSerialEnabled) {
        outputToWebSerial(formattedMessage);
    }
}

/**
 * Log debug message
 */
void DebugLogger::debug(const String& message) {
    log(LogLevel::DEBUG, message);
}

/**
 * Log info message
 */
void DebugLogger::info(const String& message) {
    log(LogLevel::INFO, message);
}

/**
 * Log warning message
 */
void DebugLogger::warning(const String& message) {
    log(LogLevel::WARNING, message);
}

/**
 * Log error message
 */
void DebugLogger::error(const String& message) {
    log(LogLevel::ERROR, message);
}

/**
 * Set minimum log level
 */
void DebugLogger::setLogLevel(LogLevel level) {
    currentLogLevel = level;
    info("Log level set to " + getLevelString(level));
}

/**
 * Get current log level
 */
DebugLogger::LogLevel DebugLogger::getLogLevel() {
    return currentLogLevel;
}

/**
 * Dump system state (calls registered callback)
 */
void DebugLogger::dumpSystemState() {
    info("=== SYSTEM STATE DUMP ===");
    
    if (stateDumpCallback) {
        String stateInfo = stateDumpCallback();
        Serial.println(stateInfo);
        if (webSerialEnabled) {
            WebSerial.println(stateInfo);
        }
    } else {
        warning("No system state dump callback registered");
    }
    
    // Basic system info
    info("Free Heap: " + String(ESP.getFreeHeap()) + " bytes");
    info("Heap Size: " + String(ESP.getHeapSize()) + " bytes");
    info("Min Free Heap: " + String(ESP.getMinFreeHeap()) + " bytes");
    info("PSRAM Size: " + String(ESP.getPsramSize()) + " bytes");
    info("Free PSRAM: " + String(ESP.getFreePsram()) + " bytes");
    info("Chip Model: " + String(ESP.getChipModel()));
    info("Chip Revision: " + String(ESP.getChipRevision()));
    info("CPU Freq: " + String(ESP.getCpuFreqMHz()) + " MHz");
    info("SDK Version: " + String(ESP.getSdkVersion()));
    info("Uptime: " + String(millis() / 1000) + " seconds");
    
    info("=== END SYSTEM STATE DUMP ===");
}

/**
 * Register callback for system state dump
 */
void DebugLogger::onSystemStateDump(SystemStateDumpCallback callback) {
    stateDumpCallback = callback;
}

/**
 * Format log message with timestamp and level
 */
String DebugLogger::formatMessage(LogLevel level, const String& message) {
    String timestamp = getTimestamp();
    String levelStr = getLevelString(level);
    
    return "[" + timestamp + "] [" + levelStr + "] " + message;
}

/**
 * Get current timestamp in format [HH:MM:SS.mmm]
 */
String DebugLogger::getTimestamp() {
    unsigned long ms = millis();
    unsigned long seconds = ms / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    
    ms = ms % 1000;
    seconds = seconds % 60;
    minutes = minutes % 60;
    hours = hours % 24;
    
    char timestamp[13];
    snprintf(timestamp, sizeof(timestamp), "%02lu:%02lu:%02lu.%03lu", 
             hours, minutes, seconds, ms);
    
    return String(timestamp);
}

/**
 * Get string representation of log level
 */
String DebugLogger::getLevelString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG:   return "DEBUG";
        case LogLevel::INFO:    return "INFO ";
        case LogLevel::WARNING: return "WARN ";
        case LogLevel::ERROR:   return "ERROR";
        default:                return "UNKN ";
    }
}

/**
 * Output message to Serial
 */
void DebugLogger::outputToSerial(const String& formattedMessage) {
    Serial.println(formattedMessage);
}

/**
 * Output message to WebSerial
 */
void DebugLogger::outputToWebSerial(const String& formattedMessage) {
    WebSerial.println(formattedMessage);
}

/**
 * Handle WebSerial messages (static callback)
 */
void DebugLogger::handleWebSerialMessage(uint8_t* data, size_t len) {
    String msg = "";
    for (size_t i = 0; i < len; i++) {
        msg += (char)data[i];
    }
    
    msg.trim();
    msg.toUpperCase();
    
    // Handle DEBUG command
    if (msg == "DEBUG" || msg == "DUMP" || msg == "STATE") {
        if (instance) {
            instance->dumpSystemState();
        }
    } else if (msg.startsWith("LOGLEVEL ")) {
        // Handle log level change: "LOGLEVEL DEBUG", "LOGLEVEL INFO", etc.
        String levelStr = msg.substring(9);
        levelStr.trim();
        
        if (instance) {
            if (levelStr == "DEBUG") {
                instance->setLogLevel(LogLevel::DEBUG);
            } else if (levelStr == "INFO") {
                instance->setLogLevel(LogLevel::INFO);
            } else if (levelStr == "WARNING" || levelStr == "WARN") {
                instance->setLogLevel(LogLevel::WARNING);
            } else if (levelStr == "ERROR") {
                instance->setLogLevel(LogLevel::ERROR);
            } else {
                instance->warning("Unknown log level: " + levelStr);
            }
        }
    } else {
        // Echo unknown commands
        if (instance) {
            instance->info("WebSerial command: " + msg);
        }
    }
}
