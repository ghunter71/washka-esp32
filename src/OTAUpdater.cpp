/**
 * OTAUpdater.cpp
 * 
 * Implementation of OTA update manager with safety checks.
 */

#include "OTAUpdater.h"
#include "StateManager.h"
#include <ESPAsyncWebServer.h>
#include <Update.h>

// Note: AsyncElegantOTA integration would be added here when library is available
// For now, we implement the safety framework and interface using ESP32 Update library

OTAUpdater::OTAUpdater() :
    webServer(nullptr),
    stateMgr(nullptr),
    progressCallback(nullptr),
    updating(false),
    currentProgress(0) {
}

OTAUpdater::~OTAUpdater() {
}

bool OTAUpdater::begin(AsyncWebServer* server, StateManager* stateManager) {
    if (server == nullptr || stateManager == nullptr) {
        Serial.println("ERROR: OTAUpdater - Invalid server or state manager");
        return false;
    }
    
    webServer = server;
    stateMgr = stateManager;
    
    // Setup OTA update endpoint with safety check
    webServer->on("/update", HTTP_GET, [this](AsyncWebServerRequest* request) {
        if (!canUpdate()) {
            request->send(503, "text/plain", 
                "OTA update blocked: System is in critical state. Please wait until cycle completes.");
            return;
        }
        
        // Serve OTA update page
        String html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>OTA Update</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial; margin: 20px; background: #1a1a1a; color: #fff; }
        .container { max-width: 600px; margin: 0 auto; }
        h1 { color: #4CAF50; }
        .warning { background: #ff9800; color: #000; padding: 15px; border-radius: 5px; margin: 20px 0; }
        .status { background: #333; padding: 15px; border-radius: 5px; margin: 20px 0; }
        input[type="file"] { margin: 20px 0; }
        button { background: #4CAF50; color: white; padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer; font-size: 16px; }
        button:hover { background: #45a049; }
        button:disabled { background: #666; cursor: not-allowed; }
        .progress { width: 100%; height: 30px; background: #333; border-radius: 5px; margin: 20px 0; overflow: hidden; }
        .progress-bar { height: 100%; background: #4CAF50; width: 0%; transition: width 0.3s; text-align: center; line-height: 30px; }
    </style>
</head>
<body>
    <div class="container">
        <h1>OTA Firmware Update</h1>
        
        <div class="warning">
            <strong>⚠️ Warning:</strong> Do not power off the device during update!
        </div>
        
        <div class="status">
            <strong>System Status:</strong> Ready for update<br>
            <strong>Current State:</strong> <span id="state">Checking...</span>
        </div>
        
        <form id="uploadForm" enctype="multipart/form-data">
            <input type="file" id="firmware" name="firmware" accept=".bin" required>
            <br>
            <button type="submit" id="uploadBtn">Upload Firmware</button>
        </form>
        
        <div class="progress" id="progressContainer" style="display:none;">
            <div class="progress-bar" id="progressBar">0%</div>
        </div>
        
        <div id="message"></div>
    </div>
    
    <script>
        // Check system state
        fetch('/api/status')
            .then(r => r.json())
            .then(data => {
                document.getElementById('state').textContent = data.status.stateDescription || 'Unknown';
            })
            .catch(e => {
                document.getElementById('state').textContent = 'Error checking state';
            });
        
        // Handle form submission
        document.getElementById('uploadForm').addEventListener('submit', function(e) {
            e.preventDefault();
            
            const fileInput = document.getElementById('firmware');
            const file = fileInput.files[0];
            
            if (!file) {
                alert('Please select a firmware file');
                return;
            }
            
            const formData = new FormData();
            formData.append('firmware', file);
            
            const progressContainer = document.getElementById('progressContainer');
            const progressBar = document.getElementById('progressBar');
            const uploadBtn = document.getElementById('uploadBtn');
            const message = document.getElementById('message');
            
            progressContainer.style.display = 'block';
            uploadBtn.disabled = true;
            message.textContent = '';
            
            const xhr = new XMLHttpRequest();
            
            xhr.upload.addEventListener('progress', function(e) {
                if (e.lengthComputable) {
                    const percent = Math.round((e.loaded / e.total) * 100);
                    progressBar.style.width = percent + '%';
                    progressBar.textContent = percent + '%';
                }
            });
            
            xhr.addEventListener('load', function() {
                if (xhr.status === 200) {
                    progressBar.style.background = '#4CAF50';
                    message.innerHTML = '<div style="color: #4CAF50; margin-top: 20px;">✓ Update successful! Device will reboot...</div>';
                    setTimeout(() => {
                        window.location.href = '/';
                    }, 5000);
                } else {
                    progressBar.style.background = '#f44336';
                    message.innerHTML = '<div style="color: #f44336; margin-top: 20px;">✗ Update failed: ' + xhr.responseText + '</div>';
                    uploadBtn.disabled = false;
                }
            });
            
            xhr.addEventListener('error', function() {
                progressBar.style.background = '#f44336';
                message.innerHTML = '<div style="color: #f44336; margin-top: 20px;">✗ Upload error</div>';
                uploadBtn.disabled = false;
            });
            
            xhr.open('POST', '/update/upload');
            xhr.send(formData);
        });
    </script>
</body>
</html>
        )";
        
        request->send(200, "text/html", html);
    });
    
    // Setup OTA upload endpoint
    webServer->on("/update/upload", HTTP_POST, 
        [this](AsyncWebServerRequest* request) {
            // This is called after upload completes
            if (!canUpdate()) {
                request->send(503, "text/plain", "Update blocked: Critical state");
                return;
            }
            
            if (Update.hasError()) {
                request->send(500, "text/plain", "Update failed");
            } else {
                request->send(200, "text/plain", "Update successful. Rebooting...");
                delay(1000);
                ESP.restart();
            }
        },
        [this](AsyncWebServerRequest* request, String filename, size_t index, uint8_t* data, size_t len, bool final) {
            // This is called during upload
            if (!canUpdate()) {
                return;
            }
            
            if (index == 0) {
                Serial.printf("OTA Update Start: %s\n", filename.c_str());
                onUpdateStart();
                
                if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
                    Update.printError(Serial);
                    onUpdateError();
                }
            }
            
            if (Update.write(data, len) != len) {
                Update.printError(Serial);
                onUpdateError();
            }
            
            onUpdateProgress(index + len, request->contentLength());
            
            if (final) {
                if (Update.end(true)) {
                    Serial.printf("OTA Update Success: %u bytes\n", index + len);
                    onUpdateEnd(true);
                } else {
                    Update.printError(Serial);
                    onUpdateEnd(false);
                    onUpdateError();
                }
            }
        }
    );
    
    Serial.println("✓ OTAUpdater initialized");
    Serial.println("  OTA update available at: /update");
    
    return true;
}

// ============================================================================
// Safety Checks
// ============================================================================

bool OTAUpdater::canUpdate() {
    if (stateMgr == nullptr) {
        Serial.println("ERROR: StateManager not initialized");
        return false;
    }
    
    // Block updates during critical states
    if (stateMgr->isCriticalState()) {
        Serial.println("OTA blocked: System in critical state");
        Serial.printf("  Current state: %s\n", stateMgr->getStateDescription().c_str());
        return false;
    }
    
    // Allow updates in IDLE, COMPLETE, or ERROR states
    if (stateMgr->isIdle() || stateMgr->isComplete() || stateMgr->isError()) {
        return true;
    }
    
    // Block in all other cases
    Serial.println("OTA blocked: System not in safe state");
    return false;
}

// ============================================================================
// Update Control
// ============================================================================

void OTAUpdater::handleUpdate() {
    // This method can be called in the main loop if needed
    // Currently, updates are handled via web endpoints
}

// ============================================================================
// Callbacks
// ============================================================================

void OTAUpdater::onProgress(ProgressCallback callback) {
    progressCallback = callback;
}

// ============================================================================
// Status
// ============================================================================

bool OTAUpdater::isUpdating() {
    return updating;
}

uint8_t OTAUpdater::getProgress() {
    return currentProgress;
}

// ============================================================================
// Internal Callbacks
// ============================================================================

void OTAUpdater::onUpdateStart() {
    updating = true;
    currentProgress = 0;
    Serial.println("OTA Update started");
}

void OTAUpdater::onUpdateProgress(size_t current, size_t total) {
    if (total > 0) {
        currentProgress = (current * 100) / total;
        
        // Call user callback if set
        if (progressCallback) {
            progressCallback(currentProgress);
        }
        
        // Log progress every 10%
        static uint8_t lastReported = 0;
        if (currentProgress >= lastReported + 10) {
            Serial.printf("OTA Progress: %d%%\n", currentProgress);
            lastReported = currentProgress;
        }
    }
}

void OTAUpdater::onUpdateEnd(bool success) {
    updating = false;
    
    if (success) {
        currentProgress = 100;
        Serial.println("✓ OTA Update completed successfully");
        
        if (progressCallback) {
            progressCallback(100);
        }
    } else {
        Serial.println("✗ OTA Update failed");
    }
}

void OTAUpdater::onUpdateError() {
    updating = false;
    Serial.println("✗ OTA Update error occurred");
}
