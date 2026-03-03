// Current configuration
let currentConfig = {};

// Initialize on page load
document.addEventListener('DOMContentLoaded', function() {
    loadTimingConfig();
    setupFormHandlers();
    setupTimeCalculation();
});

// Load current timing configuration
function loadTimingConfig() {
    fetch('/api/config')
        .then(response => response.json())
        .then(data => {
            if (data.timing) {
                currentConfig = data.timing;
                
                // Convert milliseconds to seconds for display
                document.getElementById('tpomp').value = Math.round(data.timing.tpomp / 1000);
                document.getElementById('washtime0').value = Math.round(data.timing.washtime0 / 1000);
                document.getElementById('washtime1').value = Math.round(data.timing.washtime1 / 1000);
                document.getElementById('washtime2').value = Math.round(data.timing.washtime2 / 1000);
                document.getElementById('washtime3').value = Math.round(data.timing.washtime3 / 1000);
                document.getElementById('pausa').value = Math.round(data.timing.pausa / 1000);
                document.getElementById('water_in_timer').value = Math.round(data.timing.water_in_timer / 1000);
            }
            
            if (data.gerkon) {
                document.getElementById('gerkon_threshold').value = data.gerkon.threshold || 10;
                document.getElementById('gerkon_debounce').value = data.gerkon.debounce_ms || 50;
            }
            
            calculateTotalTime();
        })
        .catch(error => {
            console.error('Failed to load timing config:', error);
            showToast('Failed to load configuration', 'danger');
        });
}

// Setup form handlers
function setupFormHandlers() {
    const form = document.getElementById('timingForm');
    
    form.addEventListener('submit', function(e) {
        e.preventDefault();
        
        const config = {
            timing: {
                tpomp: parseInt(document.getElementById('tpomp').value) * 1000,
                washtime0: parseInt(document.getElementById('washtime0').value) * 1000,
                washtime1: parseInt(document.getElementById('washtime1').value) * 1000,
                washtime2: parseInt(document.getElementById('washtime2').value) * 1000,
                washtime3: parseInt(document.getElementById('washtime3').value) * 1000,
                pausa: parseInt(document.getElementById('pausa').value) * 1000,
                water_in_timer: parseInt(document.getElementById('water_in_timer').value) * 1000
            },
            gerkon: {
                threshold: parseInt(document.getElementById('gerkon_threshold').value),
                debounce_ms: parseInt(document.getElementById('gerkon_debounce').value)
            }
        };
        
        saveTimingConfig(config);
    });
}

// Setup real-time total time calculation
function setupTimeCalculation() {
    const inputs = [
        'tpomp', 'washtime0', 'washtime1', 'washtime2', 
        'washtime3', 'pausa', 'water_in_timer'
    ];
    
    inputs.forEach(id => {
        document.getElementById(id).addEventListener('input', calculateTotalTime);
    });
}

// Calculate total estimated cycle time
function calculateTotalTime() {
    const tpomp = parseInt(document.getElementById('tpomp').value) || 0;
    const washtime0 = parseInt(document.getElementById('washtime0').value) || 0;
    const washtime1 = parseInt(document.getElementById('washtime1').value) || 0;
    const washtime2 = parseInt(document.getElementById('washtime2').value) || 0;
    const washtime3 = parseInt(document.getElementById('washtime3').value) || 0;
    const pausa = parseInt(document.getElementById('pausa').value) || 0;
    const waterTimer = parseInt(document.getElementById('water_in_timer').value) || 0;
    
    // Estimate cycle time:
    // Pre-wash: drain + fill + wash + drain
    // Main wash: fill + wash + drain
    // Rinse 1: fill + rinse + drain
    // Rinse 2: fill + rinse + drain
    // Final drain
    
    const prewashPhase = tpomp + waterTimer + washtime0 + tpomp;
    const mainWashPhase = waterTimer + washtime1 + tpomp;
    const rinse1Phase = waterTimer + washtime2 + tpomp;
    const rinse2Phase = waterTimer + washtime3 + tpomp;
    const finalDrain = tpomp;
    const totalPauses = pausa * 4; // Between major phases
    
    const totalSeconds = prewashPhase + mainWashPhase + rinse1Phase + rinse2Phase + finalDrain + totalPauses;
    
    document.getElementById('totalTime').textContent = formatTime(totalSeconds * 1000);
}

// Format milliseconds to H:MM:SS
function formatTime(ms) {
    const seconds = Math.floor(ms / 1000);
    const hours = Math.floor(seconds / 3600);
    const minutes = Math.floor((seconds % 3600) / 60);
    const secs = seconds % 60;
    
    return `${hours}:${pad(minutes)}:${pad(secs)}`;
}

// Pad number with leading zero
function pad(num) {
    return num.toString().padStart(2, '0');
}

// Save timing configuration
function saveTimingConfig(config) {
    fetch('/api/config/timing', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(config)
    })
    .then(response => response.json())
    .then(data => {
        if (data.success) {
            showToast('Timing configuration saved successfully', 'success');
            currentConfig = config;
        } else {
            showToast('Failed to save: ' + (data.error || 'Unknown error'), 'danger');
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
