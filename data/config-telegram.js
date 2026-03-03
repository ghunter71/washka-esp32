// Load configuration on page load
document.addEventListener('DOMContentLoaded', function() {
    loadTelegramConfig();
});

// Load Telegram configuration
function loadTelegramConfig() {
    fetch('/api/config')
        .then(response => response.json())
        .then(data => {
            if (data.telegram) {
                document.getElementById('telegramEnabled').checked = data.telegram.enabled || false;
                document.getElementById('botToken').value = data.telegram.token || '';
                
                // Convert chat IDs array to newline-separated string
                if (data.telegram.chatIds && data.telegram.chatIds.length > 0) {
                    document.getElementById('chatIds').value = data.telegram.chatIds.join('\n');
                } else {
                    document.getElementById('chatIds').value = '';
                }
            }
        })
        .catch(error => {
            console.error('Failed to load configuration:', error);
            showToast('Failed to load configuration', 'danger');
        });
}

// Handle form submission
document.getElementById('telegramForm').addEventListener('submit', function(e) {
    e.preventDefault();
    
    const enabled = document.getElementById('telegramEnabled').checked;
    const token = document.getElementById('botToken').value.trim();
    const chatIdsText = document.getElementById('chatIds').value.trim();
    
    // Parse chat IDs
    const chatIds = chatIdsText
        .split('\n')
        .map(id => id.trim())
        .filter(id => id.length > 0)
        .map(id => parseInt(id));
    
    // Validate
    if (enabled && !token) {
        showToast('Bot token is required when Telegram is enabled', 'warning');
        return;
    }
    
    if (enabled && chatIds.length === 0) {
        showToast('At least one chat ID is required when Telegram is enabled', 'warning');
        return;
    }
    
    // Validate chat IDs are numbers
    for (let id of chatIds) {
        if (isNaN(id)) {
            showToast('Invalid chat ID format. Must be numbers only.', 'danger');
            return;
        }
    }
    
    const config = {
        enabled: enabled,
        token: token,
        chatIds: chatIds
    };
    
    fetch('/api/config/telegram', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json'
        },
        body: JSON.stringify(config)
    })
    .then(response => response.json())
    .then(data => {
        if (data.success) {
            showToast('Telegram configuration saved successfully', 'success');
            
            if (enabled) {
                showToast('Telegram bot will be active after restart', 'info');
            }
        } else {
            showToast('Failed to save: ' + (data.error || 'Unknown error'), 'danger');
        }
    })
    .catch(error => {
        showToast('Error: ' + error.message, 'danger');
    });
});

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
    } else if (type === 'info') {
        toastEl.classList.add('bg-info', 'text-white');
    }
    
    const toast = new bootstrap.Toast(toastEl);
    toast.show();
}
