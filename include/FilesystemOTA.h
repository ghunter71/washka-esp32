#ifndef FILESYSTEM_OTA_H
#define FILESYSTEM_OTA_H

#include <ESPAsyncWebServer.h>
#include <Update.h>
#include <LittleFS.h>
#include "DebugLogger.h"

class FilesystemOTA {
private:
    AsyncWebServer* server;
    DebugLogger* logger;
    
    void handleFilesystemUpload(AsyncWebServerRequest *request, 
                                String filename, 
                                size_t index, 
                                uint8_t *data, 
                                size_t len, 
                                bool final);
    
    void handleFileUpload(AsyncWebServerRequest *request,
                         String filename,
                         size_t index,
                         uint8_t *data,
                         size_t len,
                         bool final);
    
    void handleFileList(AsyncWebServerRequest *request);
    
public:
    FilesystemOTA(AsyncWebServer* srv, DebugLogger* log);
    void begin();
};

#endif // FILESYSTEM_OTA_H
