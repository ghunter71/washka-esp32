// WebSocket connection
let ws = null;
let reconnectInterval = null;
let connectionStatus = null;

// Initialize on page load
document.addEventListener('DOMContentLoaded', function() {
    initWebSocket();
    createConnectionStatus();
    loadInitialStatus();
});

// Create connection status indicator
function createConnectionStatus() {
    connectionStatus = document.createElement('div');
    connectionStatus.className = 'connection-status connecting';
    connectionStatus.textContent = 'Connecting...';
    document.body.appendChild(connectionStatus);
}

// Initialize WebSocket connection
function initWebSocket() {
    const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
    const wsUrl = `${protocol}//${window.location.host}/ws`;
    
    try {
        ws = new WebSocket(wsUrl);
        
        ws.onopen = function() {
            console.log('WebSocket connected');
            connectionStatus.className = 'connection-status';
            connectionStatus.textContent = 'Connected';
            setTimeout(() => {
                connectionStatus.style.display = 'none';
            }, 2000);
            
            if (reconnectInterval) {
                clearInterval(reconnectInterval);
                reconnectInterval = null;
            }
        };
        
        ws.onmessage = function(event) {
            try {
                const data = JSON.parse(event.data);
                updateStatus(data);
            } catch (e) {
                console.error('Failed to parse WebSocket message:', e);
            }
        };
        
        ws.onerror = function(error) {
            console.error('WebSocket error:', error);
        };
        
        ws.onclose = function() {
            console.log('WebSocket disconnected');
            connectionStatus.style.display = 'block';
            connectionStatus.className = 'connection-status disconnected';
            connectionStatus.textContent = 'Disconnected';
            
            // Attempt to reconnect
            if (!reconnectInterval) {
                reconnectInterval = setInterval(function() {
                    console.log('Attempting to reconnect...');
                    connectionStatus.className = 'connection-status connecting';
                    connectionStatus.textContent = 'Reconnecting...';
                    initWebSocket();
                }, 5000);
            }
        };
    } catch (e) {
        console.error('Failed to create WebSocket:', e);
    }
}

// Load initial status via HTTP
function loadInitialStatus() {
    fetch('/api/status')
        .then(response => response.json())
        .then(data => updateStatus(data))
        .catch(error => console.error('Failed to load status:', error));
}

// Update UI with status data
function updateStatus(data) {
    if (data.status) {
        // Update state
        const stateEl = document.getElementById('state');
        stateEl.textContent = data.status.state || 'UNKNOWN';
        stateEl.className = 'badge ' + getStateClass(data.status.state);
        
        // Update progress
        const progress = data.status.progress || 0;
        document.getElementById('progress').textContent = progress + '%';
        const progressBar = document.getElementById('progressBar');
        progressBar.style.width = progress + '%';
        progressBar.textContent = progress + '%';
        
        // Update times
        document.getElementById('elapsed').textContent = formatTime(data.status.elapsedTime || 0);
        document.getElementById('remaining').textContent = formatTime(data.status.estimatedRemaining || 0);
        
        // Update button states
        updateButtonStates(data.status.state);
    }
    
    if (data.sensors) {
        document.getElementById('gerkonCount').textContent = data.sensors.gerkonCount || 0;
        const gerkonState = data.sensors.gerkonState ? 'HIGH' : 'LOW';
        document.getElementById('gerkonState').textContent = gerkonState;
        document.getElementById('gerkonState').className = 'badge ' + 
            (data.sensors.gerkonState ? 'bg-success' : 'bg-secondary');
    }
    
    if (data.actuators) {
        updateActuator('actWashEngine', data.actuators.washengine);
        updateActuator('actPump', data.actuators.pompa);
        updateActuator('actValve', data.actuators.waterValve);
        updateActuator('actPowder', data.actuators.powder);
        updateActuator('actLED', data.actuators.led);
    }
    
    if (data.system) {
        document.getElementById('uptime').textContent = formatTime(data.system.uptime || 0);
        document.getElementById('freeHeap').textContent = Math.round((data.system.freeHeap || 0) / 1024);
        document.getElementById('wifiRSSI').textContent = data.system.wifiRSSI || 0;
        document.getElementById('ipAddress').textContent = data.system.ipAddress || '-';
    }
}

// Get CSS class for state
function getStateClass(state) {
    if (!state) return 'bg-secondary';
    
    if (state === 'IDLE') return 'state-idle';
    if (state === 'COMPLETE') return 'state-complete';
    if (state === 'ERROR') return 'state-error';
    return 'state-running';
}

// Update actuator display
function updateActuator(elementId, state) {
    const el = document.getElementById(elementId);
    el.textContent = state ? 'ON' : 'OFF';
    el.className = 'badge ' + (state ? 'actuator-on' : 'actuator-off');
}

// Update button states based on system state
function updateButtonStates(state) {
    const btnStart = document.getElementById('btnStart');
    const btnStop = document.getElementById('btnStop');
    const btnPause = document.getElementById('btnPause');
    
    // IDLE, COMPLETE, ERROR - можно запустить
    if (state === 'Idle' || state === 'IDLE' || state === 'Complete' || state === 'COMPLETE' || state === 'Error' || state === 'ERROR') {
        btnStart.disabled = false;
        btnStop.disabled = true;
        btnPause.disabled = true;
    } 
    // Running states - можно остановить и поставить на паузу
    else {
        btnStart.disabled = true;
        btnStop.disabled = false;
        btnPause.disabled = false;
    }
}

// Format milliseconds to HH:MM:SS
function formatTime(ms) {
    const seconds = Math.floor(ms / 1000);
    const hours = Math.floor(seconds / 3600);
    const minutes = Math.floor((seconds % 3600) / 60);
    const secs = seconds % 60;
    
    return `${pad(hours)}:${pad(minutes)}:${pad(secs)}`;
}

// Pad number with leading zero
function pad(num) {
    return num.toString().padStart(2, '0');
}

// Control functions
function startCycle() {
    fetch('/api/control/start', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' }
    })
    .then(response => response.json())
    .then(data => {
        if (data.success) {
            showToast('Wash cycle started', 'success');
        } else {
            showToast('Failed to start: ' + (data.error || 'Unknown error'), 'danger');
        }
    })
    .catch(error => {
        showToast('Error: ' + error.message, 'danger');
    });
}

function stopCycle() {
    fetch('/api/control/stop', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' }
    })
    .then(response => response.json())
    .then(data => {
        if (data.success) {
            showToast('Wash cycle stopped', 'warning');
        } else {
            showToast('Failed to stop: ' + (data.error || 'Unknown error'), 'danger');
        }
    })
    .catch(error => {
        showToast('Error: ' + error.message, 'danger');
    });
}

function pauseCycle() {
    fetch('/api/control/pause', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' }
    })
    .then(response => response.json())
    .then(data => {
        if (data.success) {
            showToast('Wash cycle paused', 'info');
        } else {
            showToast('Failed to pause: ' + (data.error || 'Unknown error'), 'danger');
        }
    })
    .catch(error => {
        showToast('Error: ' + error.message, 'danger');
    });
}

// Manual actuator control
function manualControl(actuator, state) {
    fetch('/api/control/manual', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ actuator: actuator, state: state })
    })
    .then(response => response.json())
    .then(data => {
        if (data.success) {
            showToast(`${actuator} turned ${state ? 'ON' : 'OFF'}`, 'info');
        } else {
            showToast('Failed: ' + (data.error || 'Unknown error'), 'danger');
        }
    })
    .catch(error => {
        showToast('Error: ' + error.message, 'danger');
    });
}

// Show toast notification
function showToast(message, type = 'info') {
    const toastEl = document.getElementById('toast');
    const toastBody = document.getElementById('toastBody');
    
    toastBody.textContent = message;
    
    // Update toast styling based on type
    toastEl.className = 'toast';
    if (type === 'success') {
        toastEl.classList.add('bg-success', 'text-white');
    } else if (type === 'danger') {
        toastEl.classList.add('bg-danger', 'text-white');
    } else if (type === 'warning') {
        toastEl.classList.add('bg-warning');
    }
    
    const toast = new bootstrap.Toast(toastEl);
    toast.show();
}
