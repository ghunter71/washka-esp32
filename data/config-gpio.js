// WROOM-32 Pin definitions - loaded from API
let WROOM32_PINS = [];

// Current configuration
let currentConfig = {};

// Initialize on page load
document.addEventListener('DOMContentLoaded', function() {
    loadPinLegend();
});

// Load pin legend from API
function loadPinLegend() {
    fetch('/api/pins')
        .then(response => response.json())
        .then(data => {
            if (data.pins) {
                WROOM32_PINS = data.pins;
                populatePinSelects();
                loadGPIOConfig();
                setupFormHandlers();
            }
        })
        .catch(error => {
            console.error('Failed to load pin legend:', error);
            showToast('Failed to load pin information', 'danger');
        });
}

// Populate all pin select dropdowns
function populatePinSelects() {
    const selects = [
        'pinWashEngine', 'pinPump', 'pinGerkon', 
        'pinPowder', 'pinValve', 'pinButton', 'pinLED'
    ];
    
    selects.forEach(selectId => {
        const select = document.getElementById(selectId);
        
        // Clear existing options except first
        while (select.options.length > 1) {
            select.remove(1);
        }
        
        WROOM32_PINS.forEach(pin => {
            const option = document.createElement('option');
            option.value = pin.gpio;
            
            // Format: GPIO## - Label (Description)
            let text = `GPIO${pin.gpio} - ${pin.label}`;
            if (pin.description) {
                text += ` (${pin.description})`;
            }
            
            option.textContent = text;
            
            // Disable non-recommended pins
            if (!pin.recommended) {
                option.disabled = true;
                option.textContent += ' - NOT RECOMMENDED';
                option.style.color = '#dc3545';
            } else if (pin.warning && pin.warning.length > 0) {
                option.style.color = '#ffc107';
            }
            
            select.appendChild(option);
        });
        
        // Add change listener
        select.addEventListener('change', function() {
            updatePinHelp(selectId);
            checkDuplicates();
        });
    });
}

// Load current GPIO configuration
function loadGPIOConfig() {
    fetch('/api/config')
        .then(response => response.json())
        .then(data => {
            if (data.pins) {
                currentConfig = data.pins;
                
                document.getElementById('pinWashEngine').value = data.pins.washengine || '';
                document.getElementById('pinPump').value = data.pins.pompa || '';
                document.getElementById('pinGerkon').value = data.pins.watergerkon || '';
                document.getElementById('pinPowder').value = data.pins.powder || '';
                document.getElementById('pinValve').value = data.pins.water_valve || '';
                document.getElementById('pinButton').value = data.pins.button || '';
                document.getElementById('pinLED').value = data.pins.led || '';
                
                // Update help text for all pins
                updatePinHelp('pinWashEngine');
                updatePinHelp('pinPump');
                updatePinHelp('pinGerkon');
                updatePinHelp('pinPowder');
                updatePinHelp('pinValve');
                updatePinHelp('pinButton');
                updatePinHelp('pinLED');
            }
        })
        .catch(error => {
            console.error('Failed to load GPIO config:', error);
            showToast('Failed to load configuration', 'danger');
        });
}

// Update help text for a pin
function updatePinHelp(selectId) {
    const select = document.getElementById(selectId);
    const helpId = 'help' + selectId.substring(3); // Remove 'pin' prefix
    const helpEl = document.getElementById(helpId);
    
    if (!select.value) {
        helpEl.textContent = 'Select a GPIO pin';
        helpEl.className = 'form-text';
        return;
    }
    
    const pinNum = parseInt(select.value);
    const pinInfo = WROOM32_PINS.find(p => p.gpio === pinNum);
    
    if (pinInfo) {
        if (pinInfo.warning && pinInfo.warning.length > 0) {
            helpEl.textContent = '⚠️ ' + pinInfo.warning;
            helpEl.className = 'form-text text-warning';
        } else if (pinInfo.recommended) {
            let capText = '';
            if (pinInfo.capabilities && pinInfo.capabilities.length > 0) {
                capText = ' - ' + pinInfo.capabilities.join(', ');
            }
            helpEl.textContent = '✓ ' + pinInfo.description + capText;
            helpEl.className = 'form-text text-success';
        } else {
            helpEl.textContent = '✗ Not recommended for use';
            helpEl.className = 'form-text text-danger';
        }
        
        // Update pin details sidebar
        updatePinDetails(pinInfo);
    }
}

// Update pin details sidebar
function updatePinDetails(pinInfo) {
    const detailsEl = document.getElementById('pinDetails');
    
    let html = `
        <div class="mb-2">
            <strong>GPIO ${pinInfo.gpio}</strong> - ${pinInfo.label}
        </div>
        <div class="mb-2">
            <strong>Description:</strong><br>
            ${pinInfo.description}
        </div>
    `;
    
    // Capabilities
    if (pinInfo.capabilities && pinInfo.capabilities.length > 0) {
        html += `
        <div class="mb-2">
            <strong>Capabilities:</strong><br>
            <span class="text-info">${pinInfo.capabilities.join(', ')}</span>
        </div>
        `;
    }
    
    // Restrictions
    if (pinInfo.restrictions && pinInfo.restrictions.length > 0) {
        html += `
        <div class="mb-2">
            <strong>Restrictions:</strong><br>
            <span class="text-warning">${pinInfo.restrictions.join(', ')}</span>
        </div>
        `;
    }
    
    // Status
    html += `
        <div class="mb-2">
            <strong>Status:</strong><br>
            <span class="${pinInfo.recommended ? 'text-success' : 'text-danger'}">
                ${pinInfo.recommended ? '✓ Recommended' : '✗ Not recommended'}
            </span>
        </div>
    `;
    
    // Warning
    if (pinInfo.warning && pinInfo.warning.length > 0) {
        html += `
        <div class="mb-2">
            <strong>Warning:</strong><br>
            <span class="text-warning">${pinInfo.warning}</span>
        </div>
        `;
    }
    
    // Special notes
    if (pinInfo.isInputOnly) {
        html += `
        <div class="alert alert-info small p-2 mb-2">
            <strong>Input Only:</strong> This pin cannot be used for output. No pull-up/pull-down resistors available.
        </div>
        `;
    }
    
    if (pinInfo.isStrapping) {
        html += `
        <div class="alert alert-warning small p-2 mb-2">
            <strong>Strapping Pin:</strong> This pin affects boot behavior. Use with caution.
        </div>
        `;
    }
    
    if (pinInfo.isFlash) {
        html += `
        <div class="alert alert-danger small p-2 mb-2">
            <strong>Flash Pin:</strong> Connected to SPI flash. DO NOT USE.
        </div>
        `;
    }
    
    detailsEl.innerHTML = html;
}

// Check for duplicate pin assignments
function checkDuplicates() {
    const pins = [
        parseInt(document.getElementById('pinWashEngine').value),
        parseInt(document.getElementById('pinPump').value),
        parseInt(document.getElementById('pinGerkon').value),
        parseInt(document.getElementById('pinPowder').value),
        parseInt(document.getElementById('pinValve').value),
        parseInt(document.getElementById('pinButton').value),
        parseInt(document.getElementById('pinLED').value)
    ].filter(p => !isNaN(p));
    
    const duplicates = pins.filter((pin, index) => pins.indexOf(pin) !== index);
    
    const warningEl = document.getElementById('pinWarning');
    const warningText = document.getElementById('pinWarningText');
    
    if (duplicates.length > 0) {
        warningText.textContent = `Duplicate pin assignments detected: GPIO ${[...new Set(duplicates)].join(', ')}`;
        warningEl.style.display = 'block';
        return false;
    } else {
        warningEl.style.display = 'none';
        return true;
    }
}

// Setup form handlers
function setupFormHandlers() {
    const form = document.getElementById('gpioForm');
    
    form.addEventListener('submit', function(e) {
        e.preventDefault();
        
        if (!checkDuplicates()) {
            showToast('Cannot save: duplicate pin assignments', 'danger');
            return;
        }
        
        const config = {
            washengine: parseInt(document.getElementById('pinWashEngine').value),
            pompa: parseInt(document.getElementById('pinPump').value),
            watergerkon: parseInt(document.getElementById('pinGerkon').value),
            powder: parseInt(document.getElementById('pinPowder').value),
            water_valve: parseInt(document.getElementById('pinValve').value),
            button: parseInt(document.getElementById('pinButton').value),
            led: parseInt(document.getElementById('pinLED').value)
        };
        
        saveGPIOConfig(config);
    });
}

// Save GPIO configuration
function saveGPIOConfig(config) {
    fetch('/api/config', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ pins: config })
    })
    .then(response => response.json())
    .then(data => {
        if (data.success) {
            showToast('GPIO configuration saved successfully', 'success');
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
