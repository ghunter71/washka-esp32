/**
 * DebugLogger.h
 * 
 * Centralized logging system with Serial and WebSerial output.
 * Provides timestamped logging with multiple log levels and system state dumping.
 */

#ifndef DEBUG_LOGGER_H
#define DEBUG_LOGGER_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <WebSerial.h>
#include <functional>

class DebugLogger {
public:
    // Log levels
    enum class LogLevel : uint8_t {
        DEBUG = 0,
        INFO = 1,
        WARNING = 2,
        ERROR = 3
    };
    
    DebugLogger();
    ~DebugLogger();
    
    // Initialization
    bool begin(AsyncWebServer* server);
    
    // Logging methods
    void log(LogLevel level, const String& message);
    void debug(const String& message);
    void info(const String& message);
    void warning(const String& message);
    void error(const String& message);
    
    // Log level filtering
    void setLogLevel(LogLevel level);
    LogLevel getLogLevel();
    
    // System state dump
    void dumpSystemState();
    
    // Callback for system state dump (set by main.cpp)
    typedef std::function<String()> SystemStateDumpCallback;
    void onSystemStateDump(SystemStateDumpCallback callback);
    
    // Get singleton instance
    static DebugLogger& getInstance();
    
private:
    LogLevel currentLogLevel;
    AsyncWebServer* webServer;
    bool webSerialEnabled;
    SystemStateDumpCallback stateDumpCallback;
    
    // Helper methods
    String formatMessage(LogLevel level, const String& message);
    String getTimestamp();
    String getLevelString(LogLevel level);
    void outputToSerial(const String& formattedMessage);
    void outputToWebSerial(const String& formattedMessage);
    
    // WebSerial message handler
    static void handleWebSerialMessage(uint8_t* data, size_t len);
    
    // Singleton instance
    static DebugLogger* instance;
};

// Global convenience macros
#define LOG_DEBUG(msg) DebugLogger::getInstance().debug(msg)
#define LOG_INFO(msg) DebugLogger::getInstance().info(msg)
#define LOG_WARNING(msg) DebugLogger::getInstance().warning(msg)
#define LOG_ERROR(msg) DebugLogger::getInstance().error(msg)

#endif // DEBUG_LOGGER_H
