/**
 * AuthMiddleware.h
 * 
 * Authentication middleware for REST API and WebSocket.
 * Supports Bearer Token authentication with configurable token.
 * Tokens stored securely in NVS.
 * 
 * @version 1.0.0
 */

#ifndef AUTH_MIDDLEWARE_H
#define AUTH_MIDDLEWARE_H

#include <Arduino.h>
#include <Preferences.h>
#include <functional>

// Forward declaration
class AsyncWebServerRequest;

class AuthMiddleware {
public:
    // Permission levels
    enum class Permission {
        NONE = 0,
        VIEWER = 1,     // Read-only access
        USER = 2,       // Standard control access
        ADMIN = 3       // Full access including config
    };
    
    // Auth result
    struct AuthResult {
        bool success;
        Permission permission;
        String username;
        String error;
    };
    
    // Token info
    struct TokenInfo {
        String token;
        Permission permission;
        String description;
        unsigned long createdAt;
        unsigned long expiresAt;  // 0 = never expires
    };
    
    AuthMiddleware();
    ~AuthMiddleware();
    
    // Initialization
    bool begin();
    
    // Authentication methods
    AuthResult authenticate(AsyncWebServerRequest* request);
    AuthResult authenticateToken(const String& token);
    
    // Token management
    bool setAdminToken(const String& token);
    bool setUserToken(const String& token);
    bool setViewerToken(const String& token);
    String generateToken(int length = 32);
    bool revokeToken(const String& token);
    void revokeAllTokens();
    
    // Token retrieval
    String getAdminToken();
    String getUserToken();
    String getViewerToken();
    
    // Enable/disable authentication
    void setEnabled(bool enabled);
    bool isEnabled();
    
    // Permission checking
    bool hasPermission(AsyncWebServerRequest* request, Permission required);
    bool hasPermission(const String& token, Permission required);
    
    // Callbacks
    void onAuthFailed(std::function<void(const String& ip, const String& reason)> callback);
    
    // Utility
    String hashToken(const String& token);  // Simple hash for storage
    
private:
    Preferences prefs;
    bool enabled;
    String adminToken;
    String userToken;
    String viewerToken;
    
    std::function<void(const String&, const String&)> authFailedCallback;
    
    // NVS keys
    static const char* NVS_NAMESPACE;
    static const char* KEY_ENABLED;
    static const char* KEY_ADMIN_TOKEN;
    static const char* KEY_USER_TOKEN;
    static const char* KEY_VIEWER_TOKEN;
    
    // Helper methods
    String extractBearerToken(AsyncWebServerRequest* request);
    bool constantTimeCompare(const String& a, const String& b);
};

#endif // AUTH_MIDDLEWARE_H
