// Пример добавления OTA endpoint для обновления файловой системы
// Добавить в RestAPI.cpp или создать отдельный класс FilesystemOTA

#include <Update.h>
#include <LittleFS.h>

class FilesystemOTA {
private:
    AsyncWebServer* server;
    DebugLogger* logger;
    
public:
    FilesystemOTA(AsyncWebServer* srv, DebugLogger* log) 
        : server(srv), logger(log) {}
    
    void begin() {
        // Endpoint для обновления всей файловой системы (littlefs.bin)
        server->on("/api/update/filesystem", HTTP_POST,
            [this](AsyncWebServerRequest *request) {
                // Ответ отправляется в handleFilesystemUpload
            },
            [this](AsyncWebServerRequest *request, String filename, size_t index, 
                   uint8_t *data, size_t len, bool final) {
                handleFilesystemUpload(request, filename, index, data, len, final);
            }
        );
        
        // Endpoint для загрузки отдельных файлов
        server->on("/api/upload/file", HTTP_POST,
            [this](AsyncWebServerRequest *request) {
                request->send(200);
            },
            [this](AsyncWebServerRequest *request, String filename, size_t index,
                   uint8_t *data, size_t len, bool final) {
                handleFileUpload(request, filename, index, data, len, final);
            }
        );
        
        // Endpoint для удаления файлов
        server->on("/api/delete/file", HTTP_DELETE,
            [this](AsyncWebServerRequest *request) {
                if (request->hasParam("path")) {
                    String path = request->getParam("path")->value();
                    if (LittleFS.remove(path)) {
                        request->send(200, "application/json", 
                            "{\"success\":true,\"message\":\"File deleted\"}");
                    } else {
                        request->send(404, "application/json",
                            "{\"success\":false,\"message\":\"File not found\"}");
                    }
                } else {
                    request->send(400, "application/json",
                        "{\"success\":false,\"message\":\"Missing path parameter\"}");
                }
            }
        );
        
        // Endpoint для списка файлов
        server->on("/api/files/list", HTTP_GET,
            [this](AsyncWebServerRequest *request) {
                handleFileList(request);
            }
        );
    }
    
private:
    void handleFilesystemUpload(AsyncWebServerRequest *request, 
                                String filename, 
                                size_t index, 
                                uint8_t *data, 
                                size_t len, 
                                bool final) {
        if (!index) {
            logger->info("Starting filesystem update: " + filename);
            
            // Размер раздела LittleFS из partitions.csv
            size_t fsSize = 0xF0000; // 960KB
            
            if (!Update.begin(fsSize, U_SPIFFS)) {
                logger->error("Failed to begin filesystem update");
                Update.printError(Serial);
                request->send(500, "application/json",
                    "{\"success\":false,\"message\":\"Update begin failed\"}");
                return;
            }
        }
        
        // Записываем данные
        if (len) {
            if (Update.write(data, len) != len) {
                logger->error("Failed to write filesystem data");
                Update.printError(Serial);
                return;
            }
        }
        
        if (final) {
            if (Update.end(true)) {
                logger->info("Filesystem update successful, rebooting...");
                request->send(200, "application/json",
                    "{\"success\":true,\"message\":\"Filesystem updated, rebooting\"}");
                delay(1000);
                ESP.restart();
            } else {
                logger->error("Filesystem update failed");
                Update.printError(Serial);
                request->send(500, "application/json",
                    "{\"success\":false,\"message\":\"Update end failed\"}");
            }
        }
    }
    
    void handleFileUpload(AsyncWebServerRequest *request,
                         String filename,
                         size_t index,
                         uint8_t *data,
                         size_t len,
                         bool final) {
        static File uploadFile;
        
        if (!index) {
            logger->info("Starting file upload: " + filename);
            
            // Создаем директории если нужно
            String path = "/" + filename;
            int lastSlash = path.lastIndexOf('/');
            if (lastSlash > 0) {
                String dir = path.substring(0, lastSlash);
                // LittleFS не требует создания директорий
            }
            
            uploadFile = LittleFS.open(path, "w");
            if (!uploadFile) {
                logger->error("Failed to open file for writing: " + path);
                request->send(500, "application/json",
                    "{\"success\":false,\"message\":\"Failed to open file\"}");
                return;
            }
        }
        
        if (uploadFile && len) {
            size_t written = uploadFile.write(data, len);
            if (written != len) {
                logger->error("Failed to write file data");
            }
        }
        
        if (final) {
            if (uploadFile) {
                uploadFile.close();
                logger->info("File uploaded successfully: " + filename);
                request->send(200, "application/json",
                    "{\"success\":true,\"message\":\"File uploaded\"}");
            } else {
                request->send(500, "application/json",
                    "{\"success\":false,\"message\":\"Upload failed\"}");
            }
        }
    }
    
    void handleFileList(AsyncWebServerRequest *request) {
        String path = "/";
        if (request->hasParam("path")) {
            path = request->getParam("path")->value();
        }
        
        String json = "{\"files\":[";
        File root = LittleFS.open(path);
        if (root && root.isDirectory()) {
            File file = root.openNextFile();
            bool first = true;
            
            while (file) {
                if (!first) json += ",";
                first = false;
                
                json += "{";
                json += "\"name\":\"" + String(file.name()) + "\",";
                json += "\"size\":" + String(file.size()) + ",";
                json += "\"isDir\":" + String(file.isDirectory() ? "true" : "false");
                json += "}";
                
                file = root.openNextFile();
            }
        }
        json += "]}";
        
        request->send(200, "application/json", json);
    }
};

// Использование в main.cpp:
// FilesystemOTA fsOTA(&server, &logger);
// fsOTA.begin();
