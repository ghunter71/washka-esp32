// Current configuration
let currentConfig = {};
let chatIds = [];

// Initialize on page load
document.addEventListener('DOMContentLoaded', function() {
    loadWiFiConfig();
    loadTelegramConfig();
    setupFormHandlers();
});

// Load WiFi configuration
function loadWiFiConfig() {
    fetch('/api/config')
        .then(response => response.json())
        .then(data => {
            if (data.wifi) {
                document.getElementById('wifiSSID').value = data.wifi.ssid || '';
                // Don't populate password for security
            }
        })
        .catch(error => {
            console.error('Failed to load WiFi config:', error);
        });
}

// Load Telegram configuration
function loadTelegramConfig() {
    fetch('/api/config')
        .then(response => response.json())
        .then(data => {
            if (data.telegram) {
                document.getElementById('telegramToken').value = data.telegram.token || '';
                chatIds = data.telegram.chatIds || [];
                updateChatIdList();
            }
        })
        .catch(error => {
            console.error('Failed to load Telegram config:', error);
        });
}

// Update chat ID list display
function updateChatIdList() {
    const listEl = document.getElementById('chatIdList');
    
    if (chatIds.length === 0) {
        listEl.innerHTML = '<li class="list-group-item text-muted">No chat IDs configured</li>';
        return;
    }
    
    listEl.innerHTML = '';
    chatIds.forEach((chatId, index) => {
        const li = document.createElement('li');
        li.className = 'list-group-item d-flex justify-content-between align-items-center';
        li.innerHTML = `
            <span>${chatId}</span>
            <button class="btn btn-sm btn-danger" onclick="removeChatId(${index})">Remove</button>
        `;
        listEl.appendChild(li);
    });
}

// Add chat ID
function addChatId() {
    const input = document.getElementById('telegramChatId');
    const chatId = input.value.trim();
    
    if (!chatId) {
        showToast('Please enter a chat ID', 'warning');
        return;
    }
    
    if (chatIds.includes(chatId)) {
        showToast('Chat ID already exists', 'warning');
        return;
    }
    
    chatIds.push(chatId);
    updateChatIdList();
    input.value = '';
    showToast('Chat ID added', 'success');
}

// Remove chat ID
function removeChatId(index) {
    chatIds.splice(index, 1);
    updateChatIdList();
    showToast('Chat ID removed', 'info');
}

// Setup form handlers
function setupFormHandlers() {
    // WiFi form
    document.getElementById('wifiForm').addEventListener('submit', function(e) {
        e.preventDefault();
        
        const config = {
            ssid: document.getElementById('wifiSSID').value,
            password: document.getElementById('wifiPassword').value
        };
        
        saveWiFiConfig(config);
    });
    
    // Telegram form
    document.getElementById('telegramForm').addEventListener('submit', function(e) {
        e.preventDefault();
        
        const config = {
            token: document.getElementById('telegramToken').value,
            chatIds: chatIds
        };
        
        saveTelegramConfig(config);
    });
}

// Save WiFi configuration
function saveWiFiConfig(config) {
    fetch('/api/config/wifi', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(config)
    })
    .then(response => response.json())
    .then(data => {
        if (data.success) {
            showToast('WiFi configuration saved. Device will restart...', 'success');
            setTimeout(() => {
                window.location.href = '/';
            }, 3000);
        } else {
            showToast('Failed to save: ' + (data.error || 'Unknown error'), 'danger');
        }
    })
    .catch(error => {
        showToast('Error: ' + error.message, 'danger');
    });
}

// Save Telegram configuration
function saveTelegramConfig(config) {
    fetch('/api/config/telegram', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(config)
    })
    .then(response => response.json())
    .then(data => {
        if (data.success) {
            showToast('Telegram configuration saved successfully', 'success');
        } else {
            showToast('Failed to save: ' + (data.error || 'Unknown error'), 'danger');
        }
    })
    .catch(error => {
        showToast('Error: ' + error.message, 'danger');
    });
}

// Export configuration
function exportConfig() {
    fetch('/api/config/export')
        .then(response => response.json())
        .then(data => {
            const blob = new Blob([JSON.stringify(data, null, 2)], { type: 'application/json' });
            const url = URL.createObjectURL(blob);
            const a = document.createElement('a');
            a.href = url;
            a.download = 'washka-config-' + new Date().toISOString().split('T')[0] + '.json';
            document.body.appendChild(a);
            a.click();
            document.body.removeChild(a);
            URL.revokeObjectURL(url);
            showToast('Configuration exported', 'success');
        })
        .catch(error => {
            showToast('Failed to export: ' + error.message, 'danger');
        });
}

// Import configuration
function importConfig() {
    const fileInput = document.getElementById('importFile');
    const file = fileInput.files[0];
    
    if (!file) {
        showToast('Please select a file', 'warning');
        return;
    }
    
    const reader = new FileReader();
    reader.onload = function(e) {
        try {
            const config = JSON.parse(e.target.result);
            
            fetch('/api/config/import', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(config)
            })
            .then(response => response.json())
            .then(data => {
                if (data.success) {
                    showToast('Configuration imported successfully. Reloading...', 'success');
                    setTimeout(() => {
                        window.location.reload();
                    }, 2000);
                } else {
                    showToast('Failed to import: ' + (data.error || 'Unknown error'), 'danger');
                }
            })
            .catch(error => {
                showToast('Error: ' + error.message, 'danger');
            });
        } catch (error) {
            showToast('Invalid JSON file', 'danger');
        }
    };
    reader.readAsText(file);
}

// Confirm factory reset
function confirmFactoryReset() {
    const modal = new bootstrap.Modal(document.getElementById('factoryResetModal'));
    modal.show();
}

// Factory reset
function factoryReset() {
    fetch('/api/config/factory-reset', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' }
    })
    .then(response => response.json())
    .then(data => {
        if (data.success) {
            showToast('Factory reset initiated. Device will restart...', 'warning');
            setTimeout(() => {
                window.location.href = '/';
            }, 3000);
        } else {
            showToast('Failed to reset: ' + (data.error || 'Unknown error'), 'danger');
        }
    })
    .catch(error => {
        showToast('Error: ' + error.message, 'danger');
    });
    
    // Close modal
    const modal = bootstrap.Modal.getInstance(document.getElementById('factoryResetModal'));
    modal.hide();
}

// Confirm restart
function confirmRestart() {
    const modal = new bootstrap.Modal(document.getElementById('restartModal'));
    modal.show();
}

// Restart device
function restartDevice() {
    fetch('/api/system/restart', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' }
    })
    .then(response => response.json())
    .then(data => {
        if (data.success) {
            showToast('Device restarting...', 'info');
            setTimeout(() => {
                window.location.href = '/';
            }, 5000);
        } else {
            showToast('Failed to restart: ' + (data.error || 'Unknown error'), 'danger');
        }
    })
    .catch(error => {
        showToast('Error: ' + error.message, 'danger');
    });
    
    // Close modal
    const modal = bootstrap.Modal.getInstance(document.getElementById('restartModal'));
    modal.hide();
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
