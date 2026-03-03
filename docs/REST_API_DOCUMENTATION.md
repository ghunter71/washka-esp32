# Washka ESP32 REST API Documentation

## Overview

The Washka ESP32 REST API provides programmatic access to the dishwasher control system. All endpoints return JSON responses and support optional token-based authentication.

## Base URL

```
http://<device-ip>/api
```

## Authentication

The API supports optional token-based authentication. When enabled, requests must include the API token in one of the following ways:

### Bearer Token (Recommended)
```
Authorization: Bearer <your-api-token>
```

### Query Parameter
```
GET /api/status?api_token=<your-api-token>
```

### Direct Header
```
Authorization: <your-api-token>
```

## Endpoints

### GET /api/status

Returns current system status including state, progress, sensor readings, and actuator status.

**Response:**
```json
{
  "status": {
    "state": 0,
    "stateDescription": "IDLE",
    "progress": 0,
    "elapsedTime": 0,
    "estimatedRemaining": 0,
    "isIdle": true,
    "isRunning": false,
    "isPaused": false,
    "isComplete": false,
    "isError": false
  },
  "sensors": {
    "gerkonCount": 0
  },
  "actuators": {
    "washengine": false,
    "pompa": false,
    "waterValve": false,
    "powder": false,
    "led": false
  },
  "system": {
    "uptime": 123456,
    "freeHeap": 180000,
    "wifiRSSI": -65,
    "ipAddress": "192.168.1.100"
  }
}
```

### GET /api/config

Returns current system configuration including GPIO pins, timing parameters, and gerkon settings.

**Response:**
```json
{
  "pins": {
    "washengine": 5,
    "pompa": 4,
    "watergerkon": 13,
    "powder": 12,
    "water_valve": 14,
    "button": 2,
    "led": 15
  },
  "timing": {
    "tpomp": 45000,
    "washtime0": 600000,
    "washtime1": 2400000,
    "washtime2": 600000,
    "washtime3": 600000,
    "pausa": 60000,
    "water_in_timer": 180000
  },
  "gerkon": {
    "threshold": 210,
    "debounce_ms": 50
  }
}
```

### POST /api/config

Updates system configuration. Partial updates are supported - only include fields you want to change.

**Request Body:**
```json
{
  "pins": {
    "washengine": 5,
    "pompa": 4
  },
  "timing": {
    "tpomp": 50000
  },
  "gerkon": {
    "threshold": 220,
    "debounce_ms": 60
  }
}
```

**Validation Rules:**
- Pins: Must be 0-39, excluding 6-11 (flash pins)
- Timing: Must be 1000-86400000 ms (1 second to 24 hours)
- Gerkon threshold: Must be 1-1000
- Gerkon debounce: Must be 1-1000 ms

**Success Response:**
```json
{
  "success": true,
  "message": "Configuration updated successfully"
}
```

**Error Response:**
```json
{
  "error": "Invalid pin configuration. Invalid timing configuration.",
  "code": 400
}
```

### POST /api/control/start

Starts the wash cycle. System must be in IDLE state.

**Success Response:**
```json
{
  "success": true,
  "message": "Wash cycle started",
  "state": "DRAIN_PREWASH"
}
```

**Error Response:**
```json
{
  "error": "Cannot start - system is not idle",
  "code": 409
}
```

### POST /api/control/stop

Stops the wash cycle and returns to IDLE state.

**Success Response:**
```json
{
  "success": true,
  "message": "Wash cycle stopped",
  "state": "IDLE"
}
```

### POST /api/control/pause

Pauses the currently running wash cycle.

**Success Response:**
```json
{
  "success": true,
  "message": "Wash cycle paused",
  "state": "WASH"
}
```

**Error Response:**
```json
{
  "error": "Cannot pause - system is not running",
  "code": 409
}
```

### POST /api/control/resume

Resumes a paused wash cycle.

**Success Response:**
```json
{
  "success": true,
  "message": "Wash cycle resumed",
  "state": "WASH"
}
```

**Error Response:**
```json
{
  "error": "Cannot resume - system is not paused",
  "code": 409
}
```

### GET /api/docs

Returns OpenAPI 3.0 documentation for the API.

**Response:**
```json
{
  "openapi": "3.0.0",
  "info": {
    "title": "Washka System API",
    "version": "1.0.0",
    "description": "REST API for ESP32 dishwasher control system"
  },
  "paths": {
    "/api/status": { ... },
    "/api/config": { ... },
    ...
  }
}
```

## HTTP Status Codes

- `200 OK` - Request successful
- `400 Bad Request` - Invalid parameters or JSON
- `401 Unauthorized` - Missing or invalid API token
- `409 Conflict` - Operation not allowed in current state
- `500 Internal Server Error` - Server error

## Error Response Format

All error responses follow this format:

```json
{
  "error": "Error message describing what went wrong",
  "code": 400
}
```

## Example Usage

### cURL Examples

**Get Status:**
```bash
curl http://192.168.1.100/api/status
```

**Get Status with Authentication:**
```bash
curl -H "Authorization: Bearer your-token" http://192.168.1.100/api/status
```

**Start Wash Cycle:**
```bash
curl -X POST http://192.168.1.100/api/control/start
```

**Update Configuration:**
```bash
curl -X POST http://192.168.1.100/api/config \
  -H "Content-Type: application/json" \
  -d '{"timing": {"tpomp": 50000}}'
```

### Python Example

```python
import requests

# Base URL
base_url = "http://192.168.1.100/api"

# Optional: Set API token
headers = {"Authorization": "Bearer your-token"}

# Get status
response = requests.get(f"{base_url}/status", headers=headers)
status = response.json()
print(f"Current state: {status['status']['stateDescription']}")

# Start wash cycle
response = requests.post(f"{base_url}/control/start", headers=headers)
if response.status_code == 200:
    print("Wash cycle started successfully")

# Update configuration
config = {
    "gerkon": {
        "threshold": 220
    }
}
response = requests.post(f"{base_url}/config", json=config, headers=headers)
print(response.json())
```

### JavaScript Example

```javascript
const baseUrl = 'http://192.168.1.100/api';
const apiToken = 'your-token';

// Get status
fetch(`${baseUrl}/status`, {
  headers: {
    'Authorization': `Bearer ${apiToken}`
  }
})
  .then(response => response.json())
  .then(data => {
    console.log('Current state:', data.status.stateDescription);
  });

// Start wash cycle
fetch(`${baseUrl}/control/start`, {
  method: 'POST',
  headers: {
    'Authorization': `Bearer ${apiToken}`
  }
})
  .then(response => response.json())
  .then(data => {
    console.log(data.message);
  });

// Update configuration
fetch(`${baseUrl}/config`, {
  method: 'POST',
  headers: {
    'Authorization': `Bearer ${apiToken}`,
    'Content-Type': 'application/json'
  },
  body: JSON.stringify({
    gerkon: {
      threshold: 220
    }
  })
})
  .then(response => response.json())
  .then(data => {
    console.log(data.message);
  });
```

## Security Considerations

1. **Enable Authentication**: Always set an API token in production environments
2. **Use HTTPS**: Consider using a reverse proxy with HTTPS for secure communication
3. **Network Isolation**: Keep the device on a trusted network
4. **Token Rotation**: Periodically change the API token
5. **Rate Limiting**: The API does not implement rate limiting - consider adding this at the network level

## Integration with Other Components

The REST API integrates with:
- **ConfigManager**: For reading and updating configuration
- **StateManager**: For controlling wash cycles and reading state
- **ActuatorManager**: For reading actuator status
- **WaterControl**: For reading gerkon sensor data

## Troubleshooting

**401 Unauthorized:**
- Check that the API token is correct
- Verify the Authorization header format
- Ensure authentication is enabled if using a token

**400 Bad Request:**
- Validate JSON syntax
- Check parameter ranges
- Ensure all required fields are present

**409 Conflict:**
- Check current system state
- Ensure the requested operation is valid for the current state

**Connection Refused:**
- Verify device IP address
- Check that the device is powered on and connected to WiFi
- Ensure the web server is running
