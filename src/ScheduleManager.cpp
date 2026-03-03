/**
 * ScheduleManager.cpp
 * 
 * Implementation of schedule management.
 * 
 * @version 1.0.0
 */

#include "ScheduleManager.h"
#include "ConfigManager.h"
#include "StateManager.h"
#include <Preferences.h>
#include <ArduinoJson.h>

const char* ScheduleManager::NVS_NAMESPACE = "washka_sched";

ScheduleManager::ScheduleManager() 
    : configManager(nullptr),
      stateManager(nullptr),
      hasDelayedStartActive(false),
      triggerCallback(nullptr),
      timeSynced(false),
      lastTimeSync(0),
      lastScheduleCheck(0) {
}

ScheduleManager::~ScheduleManager() {
    schedules.clear();
}

bool ScheduleManager::begin(ConfigManager* config, StateManager* state) {
    configManager = config;
    stateManager = state;
    
    // Load saved schedules
    loadSchedules();
    
    // Try to sync time
    syncTime();
    
    Serial.println("✓ ScheduleManager initialized");
    return true;
}

// ============================================================================
// Time Synchronization
// ============================================================================

bool ScheduleManager::syncTime() {
    Serial.println("ScheduleManager: Syncing time via NTP...");
    
    configTime(gmtOffsetSec, daylightOffsetSec, ntpServer);
    
    // Wait for time sync (max 10 seconds)
    int attempts = 0;
    while (time(nullptr) < 1000000000 && attempts < 20) {
        delay(500);
        attempts++;
    }
    
    time_t now = time(nullptr);
    timeSynced = (now > 1000000000);
    lastTimeSync = millis();
    
    if (timeSynced) {
        Serial.printf("✓ Time synced: %s", ctime(&now));
    } else {
        Serial.println("⚠ Time sync failed, schedules may not work correctly");
    }
    
    return timeSynced;
}

bool ScheduleManager::isTimeSynced() {
    // Periodically re-sync
    if (millis() - lastTimeSync > TIME_SYNC_INTERVAL) {
        syncTime();
    }
    return timeSynced;
}

String ScheduleManager::getCurrentTimeString() {
    if (!isTimeSynced()) {
        return "Not synced";
    }
    
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    
    char buffer[32];
    strftime(buffer, sizeof(buffer), "%H:%M:%S %d.%m.%Y", timeinfo);
    
    return String(buffer);
}

time_t ScheduleManager::getCurrentTime() {
    return time(nullptr);
}

// ============================================================================
// Schedule Management
// ============================================================================

uint8_t ScheduleManager::addSchedule(const ScheduleEntry& schedule) {
    // Find next available ID
    uint8_t maxId = 0;
    for (const auto& s : schedules) {
        if (s.id > maxId) {
            maxId = s.id;
        }
    }
    
    ScheduleEntry newSchedule = schedule;
    newSchedule.id = maxId + 1;
    newSchedule.nextExecution = calculateNextExecution(newSchedule);
    
    schedules.push_back(newSchedule);
    saveSchedules();
    
    Serial.printf("✓ Schedule added: ID=%d, type=%d, time=%02d:%02d\n", 
                  newSchedule.id, (int)newSchedule.type, 
                  newSchedule.hour, newSchedule.minute);
    
    return newSchedule.id;
}

bool ScheduleManager::updateSchedule(uint8_t id, const ScheduleEntry& schedule) {
    for (auto& s : schedules) {
        if (s.id == id) {
            s = schedule;
            s.id = id;  // Preserve ID
            s.nextExecution = calculateNextExecution(s);
            saveSchedules();
            
            Serial.printf("✓ Schedule updated: ID=%d\n", id);
            return true;
        }
    }
    return false;
}

bool ScheduleManager::removeSchedule(uint8_t id) {
    for (auto it = schedules.begin(); it != schedules.end(); ++it) {
        if (it->id == id) {
            schedules.erase(it);
            saveSchedules();
            
            Serial.printf("✓ Schedule removed: ID=%d\n", id);
            return true;
        }
    }
    return false;
}

void ScheduleManager::clearAllSchedules() {
    schedules.clear();
    saveSchedules();
    Serial.println("✓ All schedules cleared");
}

std::vector<ScheduleManager::ScheduleEntry> ScheduleManager::getSchedules() {
    return schedules;
}

ScheduleManager::ScheduleEntry* ScheduleManager::getSchedule(uint8_t id) {
    for (auto& s : schedules) {
        if (s.id == id) {
            return &s;
        }
    }
    return nullptr;
}

// ============================================================================
// Delayed Start
// ============================================================================

bool ScheduleManager::setDelayedStart(unsigned long delayMinutes) {
    if (!isTimeSynced()) {
        Serial.println("ERROR: Cannot set delayed start - time not synced");
        return false;
    }
    
    time_t triggerTime = time(nullptr) + (delayMinutes * 60);
    struct tm* timeinfo = localtime(&triggerTime);
    
    delayedStart.type = ScheduleType::ONCE;
    delayedStart.hour = timeinfo->tm_hour;
    delayedStart.minute = timeinfo->tm_min;
    delayedStart.enabled = true;
    delayedStart.nextExecution = triggerTime;
    hasDelayedStartActive = true;
    
    Serial.printf("✓ Delayed start set: %02d:%02d (in %lu minutes)\n",
                  delayedStart.hour, delayedStart.minute, delayMinutes);
    
    return true;
}

bool ScheduleManager::setDelayedStartTime(uint8_t hour, uint8_t minute) {
    if (!isTimeSynced()) {
        Serial.println("ERROR: Cannot set delayed start - time not synced");
        return false;
    }
    
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    
    // Calculate trigger time for today or tomorrow
    struct tm triggerTm = *timeinfo;
    triggerTm.tm_hour = hour;
    triggerTm.tm_min = minute;
    triggerTm.tm_sec = 0;
    
    time_t triggerTime = mktime(&triggerTm);
    
    // If time has passed today, schedule for tomorrow
    if (triggerTime <= now) {
        triggerTm.tm_mday++;
        triggerTime = mktime(&triggerTm);
    }
    
    delayedStart.type = ScheduleType::ONCE;
    delayedStart.hour = hour;
    delayedStart.minute = minute;
    delayedStart.enabled = true;
    delayedStart.nextExecution = triggerTime;
    hasDelayedStartActive = true;
    
    Serial.printf("✓ Delayed start set for: %02d:%02d\n", hour, minute);
    
    return true;
}

void ScheduleManager::cancelDelayedStart() {
    delayedStart.enabled = false;
    hasDelayedStartActive = false;
    Serial.println("✓ Delayed start cancelled");
}

bool ScheduleManager::hasDelayedStart() {
    return hasDelayedStartActive && delayedStart.enabled;
}

unsigned long ScheduleManager::getDelayedStartRemaining() {
    if (!hasDelayedStart()) {
        return 0;
    }
    
    time_t now = time(nullptr);
    if (delayedStart.nextExecution <= now) {
        return 0;
    }
    
    return (delayedStart.nextExecution - now) * 1000;  // Return milliseconds
}

// ============================================================================
// Main Loop
// ============================================================================

void ScheduleManager::loop() {
    unsigned long now = millis();
    
    // Check time sync
    if (!isTimeSynced()) {
        return;
    }
    
    // Check schedules periodically
    if (now - lastScheduleCheck >= SCHEDULE_CHECK_INTERVAL) {
        lastScheduleCheck = now;
        checkSchedules();
    }
}

// ============================================================================
// Callbacks
// ============================================================================

void ScheduleManager::onScheduleTrigger(ScheduleTriggerCallback callback) {
    triggerCallback = callback;
}

// ============================================================================
// Statistics
// ============================================================================

void ScheduleManager::recordCycleStart() {
    // Called when cycle starts
}

void ScheduleManager::recordCycleEnd(unsigned long duration, unsigned long waterCount) {
    stats.totalCycles++;
    stats.totalWaterCount += waterCount;
    stats.totalRuntime += duration;
    stats.lastCycleTime = time(nullptr);
    stats.averageCycleTime = stats.totalRuntime / stats.totalCycles;
}

ScheduleManager::CycleStats ScheduleManager::getStatistics() {
    return stats;
}

void ScheduleManager::resetStatistics() {
    stats = CycleStats();
}

// ============================================================================
// Persistence
// ============================================================================

bool ScheduleManager::saveSchedules() {
    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, false)) {
        return false;
    }
    
    // Save count
    prefs.putUChar("count", schedules.size());
    
    // Save each schedule
    for (size_t i = 0; i < schedules.size(); i++) {
        String prefix = "s" + String(i) + "_";
        prefs.putUChar((prefix + "id").c_str(), schedules[i].id);
        prefs.putUChar((prefix + "type").c_str(), (uint8_t)schedules[i].type);
        prefs.putUChar((prefix + "hour").c_str(), schedules[i].hour);
        prefs.putUChar((prefix + "min").c_str(), schedules[i].minute);
        prefs.putUChar((prefix + "days").c_str(), schedules[i].daysOfWeek);
        prefs.putBool((prefix + "en").c_str(), schedules[i].enabled);
    }
    
    prefs.end();
    return true;
}

bool ScheduleManager::loadSchedules() {
    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, true)) {
        return false;
    }
    
    uint8_t count = prefs.getUChar("count", 0);
    schedules.clear();
    
    for (uint8_t i = 0; i < count; i++) {
        String prefix = "s" + String(i) + "_";
        ScheduleEntry entry;
        
        entry.id = prefs.getUChar((prefix + "id").c_str(), 0);
        entry.type = (ScheduleType)prefs.getUChar((prefix + "type").c_str(), 0);
        entry.hour = prefs.getUChar((prefix + "hour").c_str(), 0);
        entry.minute = prefs.getUChar((prefix + "min").c_str(), 0);
        entry.daysOfWeek = prefs.getUChar((prefix + "days").c_str(), 0x7F);
        entry.enabled = prefs.getBool((prefix + "en").c_str(), true);
        entry.nextExecution = calculateNextExecution(entry);
        
        schedules.push_back(entry);
    }
    
    prefs.end();
    
    Serial.printf("✓ Loaded %d schedules\n", count);
    return true;
}

// ============================================================================
// Helper Methods
// ============================================================================

void ScheduleManager::checkSchedules() {
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    
    // Check delayed start first
    if (hasDelayedStart() && delayedStart.nextExecution <= now) {
        Serial.println("⏰ Delayed start triggered!");
        triggerSchedule(delayedStart);
        cancelDelayedStart();
        return;
    }
    
    // Check regular schedules
    for (auto& schedule : schedules) {
        if (!schedule.enabled) continue;
        
        if (schedule.nextExecution <= now) {
            // Check if day of week matches for weekly schedules
            if (schedule.type == ScheduleType::WEEKLY || 
                schedule.type == ScheduleType::DAILY) {
                if (!isDayOfWeekMatch(schedule.daysOfWeek, timeinfo->tm_wday)) {
                    // Calculate next execution for this schedule
                    schedule.nextExecution = calculateNextExecution(schedule);
                    continue;
                }
            }
            
            Serial.printf("⏰ Schedule triggered: ID=%d\n", schedule.id);
            triggerSchedule(schedule);
            
            // Calculate next execution
            schedule.nextExecution = calculateNextExecution(schedule);
            
            // For ONCE type, disable after triggering
            if (schedule.type == ScheduleType::ONCE) {
                schedule.enabled = false;
            }
        }
    }
}

unsigned long ScheduleManager::calculateNextExecution(const ScheduleEntry& schedule) {
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    
    struct tm nextTm = *timeinfo;
    nextTm.tm_hour = schedule.hour;
    nextTm.tm_min = schedule.minute;
    nextTm.tm_sec = 0;
    
    time_t nextTime = mktime(&nextTm);
    
    // If time has passed today, move to next day
    if (nextTime <= now) {
        nextTm.tm_mday++;
        nextTime = mktime(&nextTm);
    }
    
    // For weekly, find next matching day
    if (schedule.type == ScheduleType::WEEKLY) {
        while (!isDayOfWeekMatch(schedule.daysOfWeek, localtime(&nextTime)->tm_wday)) {
            nextTm.tm_mday++;
            nextTime = mktime(&nextTm);
        }
    }
    
    return nextTime;
}

bool ScheduleManager::isDayOfWeekMatch(uint8_t daysOfWeek, int currentDayOfWeek) {
    // daysOfWeek: bit 0 = Sunday, bit 6 = Saturday
    // currentDayOfWeek: 0 = Sunday, 6 = Saturday
    return (daysOfWeek & (1 << currentDayOfWeek)) != 0;
}

void ScheduleManager::triggerSchedule(const ScheduleEntry& schedule) {
    if (triggerCallback) {
        triggerCallback(schedule);
    }
    
    // Record for statistics
    recordCycleStart();
}
