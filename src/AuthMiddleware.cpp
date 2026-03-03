/**
 * AuthMiddleware.cpp
 * 
 * Implementation of authentication middleware.
 * Provides token-based authentication for API endpoints.
 * 
 * @version 1.0.0
 */

#include "AuthMiddleware.h"
#include <ESPAsyncWebServer.h>
#include <mbedtls/md.h>

// NVS keys
const char* AuthMiddleware::NVS_NAMESPACE = "washka_auth";
const char* AuthMiddleware::KEY_ENABLED = "auth_enabled";
const char* AuthMiddleware::KEY_ADMIN_TOKEN = "admin_token";
const char* AuthMiddleware::KEY_USER_TOKEN = "user_token";
const char* AuthMiddleware::KEY_VIEWER_TOKEN = "viewer_token";

AuthMiddleware::AuthMiddleware() 
    : enabled(false), authFailedCallback(nullptr) {
}

AuthMiddleware::~AuthMiddleware() {
}

bool AuthMiddleware::begin() {
    // Load settings from NVS
    if (!prefs.begin(NVS_NAMESPACE, true)) {
        Serial.println("ERROR: Failed to open Auth NVS namespace");
        return false;
    }
    
    enabled = prefs.getBool(KEY_ENABLED, false);
    adminToken = prefs.getString(KEY_ADMIN_TOKEN, "");
    userToken = prefs.getString(KEY_USER_TOKEN, "");
    viewerToken = prefs.getString(KEY_VIEWER_TOKEN, "");
    
    prefs.end();
    
    // Generate default admin token if none exists
    if (adminToken.isEmpty()) {
        adminToken = generateToken(32);
        setAdminToken(adminToken);
        Serial.println("⚠ Generated new admin token (save this!):");
        Serial.println(adminToken);
    }
    
    Serial.printf("✓ AuthMiddleware initialized (auth %s)\n", enabled ? "ENABLED" : "disabled");
    return true;
}

// ============================================================================
// Authentication
// ============================================================================

AuthMiddleware::AuthResult AuthMiddleware::authenticate(AsyncWebServerRequest* request) {
    AuthResult result = {false, Permission::NONE, "", ""};
    
    if (!enabled) {
        // Authentication disabled, grant admin access
        result.success = true;
        result.permission = Permission::ADMIN;
        result.username = "anonymous";
        return result;
    }
    
    // Extract Bearer token
    String token = extractBearerToken(request);
    
    if (token.isEmpty()) {
        result.error = "Missing or invalid Authorization header";
        
        if (authFailedCallback) {
            authFailedCallback(request->client()->remoteIP().toString(), result.error);
        }
        
        return result;
    }
    
    return authenticateToken(token);
}

AuthMiddleware::AuthResult AuthMiddleware::authenticateToken(const String& token) {
    AuthResult result = {false, Permission::NONE, "", ""};
    
    if (!enabled) {
        result.success = true;
        result.permission = Permission::ADMIN;
        result.username = "anonymous";
        return result;
    }
    
    // Check admin token first (highest permission)
    if (!adminToken.isEmpty() && constantTimeCompare(token, adminToken)) {
        result.success = true;
        result.permission = Permission::ADMIN;
        result.username = "admin";
        return result;
    }
    
    // Check user token
    if (!userToken.isEmpty() && constantTimeCompare(token, userToken)) {
        result.success = true;
        result.permission = Permission::USER;
        result.username = "user";
        return result;
    }
    
    // Check viewer token
    if (!viewerToken.isEmpty() && constantTimeCompare(token, viewerToken)) {
        result.success = true;
        result.permission = Permission::VIEWER;
        result.username = "viewer";
        return result;
    }
    
    result.error = "Invalid or expired token";
    
    if (authFailedCallback) {
        authFailedCallback("", result.error);
    }
    
    return result;
}

// ============================================================================
// Token Management
// ============================================================================

bool AuthMiddleware::setAdminToken(const String& token) {
    if (!prefs.begin(NVS_NAMESPACE, false)) {
        return false;
    }
    
    prefs.putString(KEY_ADMIN_TOKEN, token);
    adminToken = token;
    
    prefs.end();
    Serial.println("✓ Admin token updated");
    return true;
}

bool AuthMiddleware::setUserToken(const String& token) {
    if (!prefs.begin(NVS_NAMESPACE, false)) {
        return false;
    }
    
    prefs.putString(KEY_USER_TOKEN, token);
    userToken = token;
    
    prefs.end();
    Serial.println("✓ User token updated");
    return true;
}

bool AuthMiddleware::setViewerToken(const String& token) {
    if (!prefs.begin(NVS_NAMESPACE, false)) {
        return false;
    }
    
    prefs.putString(KEY_VIEWER_TOKEN, token);
    viewerToken = token;
    
    prefs.end();
    Serial.println("✓ Viewer token updated");
    return true;
}

String AuthMiddleware::generateToken(int length) {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    String token = "";
    
    for (int i = 0; i < length; i++) {
        token += charset[random(0, sizeof(charset) - 1)];
    }
    
    return token;
}

bool AuthMiddleware::revokeToken(const String& token) {
    // Check which token it is and clear it
    if (constantTimeCompare(token, adminToken)) {
        setAdminToken("");
        return true;
    }
    if (constantTimeCompare(token, userToken)) {
        setUserToken("");
        return true;
    }
    if (constantTimeCompare(token, viewerToken)) {
        setViewerToken("");
        return true;
    }
    return false;
}

void AuthMiddleware::revokeAllTokens() {
    if (prefs.begin(NVS_NAMESPACE, false)) {
        prefs.remove(KEY_ADMIN_TOKEN);
        prefs.remove(KEY_USER_TOKEN);
        prefs.remove(KEY_VIEWER_TOKEN);
        prefs.end();
    }
    
    adminToken = "";
    userToken = "";
    viewerToken = "";
    
    Serial.println("✓ All tokens revoked");
}

String AuthMiddleware::getAdminToken() {
    return adminToken;
}

String AuthMiddleware::getUserToken() {
    return userToken;
}

String AuthMiddleware::getViewerToken() {
    return viewerToken;
}

// ============================================================================
// Enable/Disable
// ============================================================================

void AuthMiddleware::setEnabled(bool enable) {
    if (prefs.begin(NVS_NAMESPACE, false)) {
        prefs.putBool(KEY_ENABLED, enable);
        prefs.end();
    }
    
    enabled = enable;
    Serial.printf("✓ Authentication %s\n", enable ? "ENABLED" : "disabled");
}

bool AuthMiddleware::isEnabled() {
    return enabled;
}

// ============================================================================
// Permission Checking
// ============================================================================

bool AuthMiddleware::hasPermission(AsyncWebServerRequest* request, Permission required) {
    AuthResult result = authenticate(request);
    return result.success && result.permission >= required;
}

bool AuthMiddleware::hasPermission(const String& token, Permission required) {
    AuthResult result = authenticateToken(token);
    return result.success && result.permission >= required;
}

// ============================================================================
// Callbacks
// ============================================================================

void AuthMiddleware::onAuthFailed(std::function<void(const String& ip, const String& reason)> callback) {
    authFailedCallback = callback;
}

// ============================================================================
// Utility
// ============================================================================

String AuthMiddleware::hashToken(const String& token) {
    byte shaResult[32];
    
    mbedtls_md_context_t ctx;
    mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
    
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 0);
    mbedtls_md_starts(&ctx);
    mbedtls_md_update(&ctx, (const unsigned char*)token.c_str(), token.length());
    mbedtls_md_finish(&ctx, shaResult);
    mbedtls_md_free(&ctx);
    
    String hash = "";
    for (int i = 0; i < 32; i++) {
        char hex[3];
        sprintf(hex, "%02x", shaResult[i]);
        hash += hex;
    }
    
    return hash;
}

String AuthMiddleware::extractBearerToken(AsyncWebServerRequest* request) {
    // Check Authorization header
    if (request->hasHeader("Authorization")) {
        String authHeader = request->getHeader("Authorization")->value();
        if (authHeader.startsWith("Bearer ")) {
            return authHeader.substring(7);
        }
    }
    
    // Check URL parameter as fallback
    if (request->hasParam("token")) {
        return request->getParam("token")->value();
    }
    
    // Check X-API-Key header
    if (request->hasHeader("X-API-Key")) {
        return request->getHeader("X-API-Key")->value();
    }
    
    return "";
}

bool AuthMiddleware::constantTimeCompare(const String& a, const String& b) {
    if (a.length() != b.length()) {
        return false;
    }
    
    volatile int result = 0;
    for (size_t i = 0; i < a.length(); i++) {
        result |= (a[i] ^ b[i]);
    }
    
    return result == 0;
}
