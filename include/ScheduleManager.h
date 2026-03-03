/**
 * ScheduleManager.h
 * 
 * Schedule management for delayed start and recurring wash cycles.
 * Supports cron-like scheduling and one-time delayed starts.
 * 
 * @version 1.0.0
 */

#ifndef SCHEDULE_MANAGER_H
#define SCHEDULE_MANAGER_H

#include <Arduino.h>
#include <vector>
#include <functional>
#include <time.h>

// Forward declarations
class ConfigManager;
class StateManager;

class ScheduleManager {
public:
    // Schedule types
    enum class ScheduleType {
        ONCE,           // One-time execution
        DAILY,          // Every day at specified time
        WEEKLY,         // Weekly on specified days
        INTERVAL        // Periodic interval
    };
    
    // Schedule entry
    struct ScheduleEntry {
        uint8_t id;
        ScheduleType type;
        uint8_t hour;
        uint8_t minute;
        uint8_t daysOfWeek;    // Bitmask: bit 0 = Sunday, bit 6 = Saturday
        unsigned long intervalMinutes;  // For INTERVAL type
        bool enabled;
        String programName;    // Optional program name
        unsigned long nextExecution;
        
        ScheduleEntry() :
            id(0),
            type(ScheduleType::ONCE),
            hour(0),
            minute(0),
            daysOfWeek(0x7F),  // All days
            intervalMinutes(0),
            enabled(true),
            nextExecution(0) {}
    };
    
    // Statistics
    struct CycleStats {
        unsigned long totalCycles;
        unsigned long totalWaterCount;
        unsigned long totalRuntime;
        time_t lastCycleTime;
        unsigned long averageCycleTime;
        
        CycleStats() :
            totalCycles(0),
            totalWaterCount(0),
            totalRuntime(0),
            lastCycleTime(0),
            averageCycleTime(0) {}
    };
    
    // Callback types
    typedef std::function<void(const ScheduleEntry& schedule)> ScheduleTriggerCallback;
    
    ScheduleManager();
    ~ScheduleManager();
    
    // Initialization
    bool begin(ConfigManager* config, StateManager* state);
    
    // Time synchronization
    bool syncTime();
    bool isTimeSynced();
    String getCurrentTimeString();
    time_t getCurrentTime();
    
    // Schedule management
    uint8_t addSchedule(const ScheduleEntry& schedule);
    bool updateSchedule(uint8_t id, const ScheduleEntry& schedule);
    bool removeSchedule(uint8_t id);
    void clearAllSchedules();
    std::vector<ScheduleEntry> getSchedules();
    ScheduleEntry* getSchedule(uint8_t id);
    
    // Delayed start
    bool setDelayedStart(unsigned long delayMinutes);
    bool setDelayedStartTime(uint8_t hour, uint8_t minute);
    void cancelDelayedStart();
    bool hasDelayedStart();
    unsigned long getDelayedStartRemaining();
    
    // Main loop - call from main loop
    void loop();
    
    // Callbacks
    void onScheduleTrigger(ScheduleTriggerCallback callback);
    
    // Statistics
    void recordCycleStart();
    void recordCycleEnd(unsigned long duration, unsigned long waterCount);
    CycleStats getStatistics();
    void resetStatistics();
    
    // Persistence
    bool saveSchedules();
    bool loadSchedules();
    
private:
    ConfigManager* configManager;
    StateManager* stateManager;
    
    std::vector<ScheduleEntry> schedules;
    ScheduleEntry delayedStart;
    bool hasDelayedStartActive;
    
    ScheduleTriggerCallback triggerCallback;
    
    bool timeSynced;
    unsigned long lastTimeSync;
    unsigned long lastScheduleCheck;
    
    CycleStats stats;
    
    // Timing
    static const unsigned long TIME_SYNC_INTERVAL = 3600000;  // 1 hour
    static const unsigned long SCHEDULE_CHECK_INTERVAL = 10000;  // 10 seconds
    
    // NTP settings
    const char* ntpServer = "pool.ntp.org";
    const long gmtOffsetSec = 0;
    const int daylightOffsetSec = 0;
    
    // Helper methods
    void checkSchedules();
    unsigned long calculateNextExecution(const ScheduleEntry& schedule);
    bool isDayOfWeekMatch(uint8_t daysOfWeek, int currentDayOfWeek);
    void triggerSchedule(const ScheduleEntry& schedule);
    
    // NVS keys
    static const char* NVS_NAMESPACE;
};

#endif // SCHEDULE_MANAGER_H
