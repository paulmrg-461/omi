#ifndef STANDALONE_H
#define STANDALONE_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include "config.h"
#include "mic.h"

// Global objects
WebSocketsClient webSocket;
String deviceId = "";
bool isConnectedToBackend = false;

// Audio callback to send data via WebSocket
void standaloneAudioCallback(int16_t *data, size_t samples) {
    if (isConnectedToBackend) {
        // Send binary data
        webSocket.sendBIN((uint8_t*)data, samples * 2);
    }
}

// WebSocket event handler
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
    switch(type) {
        case WStype_DISCONNECTED:
            Serial.printf("[WSc] Disconnected!\n");
            break;
        case WStype_CONNECTED:
            Serial.printf("[WSc] Connected to url: %s\n", payload);
            break;
        case WStype_TEXT:
            Serial.printf("[WSc] get text: %s\n", payload);
            break;
        case WStype_BIN:
            Serial.printf("[WSc] get binary length: %u\n", length);
            break;
        case WStype_ERROR:      
        case WStype_FRAGMENT_TEXT_START:
        case WStype_FRAGMENT_BIN_START:
        case WStype_FRAGMENT:
        case WStype_FRAGMENT_FIN:
            break;
    }
}

void setupStandalone() {
    Serial.println("Starting Standalone Mode...");
    
    // 1. Connect to WiFi
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.printf("Connecting to WiFi SSID: %s\n", WIFI_SSID);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        attempts++;
        
        if (attempts > 20) { // 10 seconds
            Serial.println("\nWiFi connection timed out.");
            Serial.println("Scanning for available networks...");
            int n = WiFi.scanNetworks();
            Serial.println("Scan done");
            if (n == 0) {
                Serial.println("no networks found");
            } else {
                Serial.print(n);
                Serial.println(" networks found");
                for (int i = 0; i < n; ++i) {
                    Serial.print(i + 1);
                    Serial.print(": ");
                    Serial.print(WiFi.SSID(i));
                    Serial.print(" (");
                    Serial.print(WiFi.RSSI(i));
                    Serial.print(")");
                    Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?" ":"*");
                    delay(10);
                }
            }
            // Retry
            attempts = 0;
            WiFi.begin(WIFI_SSID, WIFI_PASS);
            Serial.printf("Retrying connection to %s...\n", WIFI_SSID);
        }
    }
    Serial.println("\nWiFi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    // 2. Register Device via HTTP
    if (WiFi.status() == WL_CONNECTED) {
        WiFiClientSecure client;
        client.setInsecure(); // Skip certificate validation for devtunnels

        HTTPClient http;
        String registerUrl = String(API_BASE_URL) + "/devices/connect";
        
        Serial.println("Registering device at: " + registerUrl);
        
        // Use client for secure connection
        http.begin(client, registerUrl);
        http.addHeader("Content-Type", "application/json");
        
        // Create JSON payload
        StaticJsonDocument<200> doc;
        doc["mac_address"] = WiFi.macAddress();
        doc["model"] = "XIAO_ESP32S3";
        doc["firmware_version"] = FIRMWARE_VERSION_STRING;
        
        String requestBody;
        serializeJson(doc, requestBody);
        
        int httpResponseCode = http.POST(requestBody);
        
        if (httpResponseCode > 0) {
            String response = http.getString();
            Serial.println("Registration successful: " + response);
            
            // Parse response to get ID
            StaticJsonDocument<512> responseDoc;
            DeserializationError error = deserializeJson(responseDoc, response);
            
            if (!error) {
                const char* id = responseDoc["id"];
                deviceId = String(id);
                Serial.println("Device ID: " + deviceId);
                
                // 3. Setup WebSocket
                String wsPath = "/ws/audio/stream/" + deviceId;
                
                // Extract host and port from API_WS_URL if needed, but WebSocketsClient takes host, port, path
                // Parsing API_WS_URL: ws://host:port
                String wsUrl = String(API_WS_URL);
                String host;
                int port = 80;
                
                // Simple parsing (assuming ws:// protocol)
                int protocolEnd = wsUrl.indexOf("://");
                int portStart = wsUrl.lastIndexOf(":");
                
                if (portStart > protocolEnd) {
                    // Port is specified
                    host = wsUrl.substring(protocolEnd + 3, portStart);
                    port = wsUrl.substring(portStart + 1).toInt();
                } else {
                    // No port, default to 80 (or 443 if wss)
                    host = wsUrl.substring(protocolEnd + 3);
                    if (wsUrl.startsWith("wss")) port = 443;
                }

                // Remove trailing slash from host if present
                if (host.endsWith("/")) host.remove(host.length() - 1);

                Serial.printf("Connecting WebSocket to %s:%d%s\n", host.c_str(), port, wsPath.c_str());
                
                if (wsUrl.startsWith("wss")) {
                    webSocket.beginSSL(host.c_str(), port, wsPath.c_str());
                } else {
                    webSocket.begin(host.c_str(), port, wsPath.c_str());
                }
                webSocket.onEvent(webSocketEvent);
                webSocket.setReconnectInterval(5000);
                
                isConnectedToBackend = true;
                
            } else {
                Serial.print("JSON parsing failed: ");
                Serial.println(error.c_str());
            }
        } else {
            Serial.print("Error on sending POST: ");
            Serial.println(httpResponseCode);
        }
        
        http.end();
    }
}

void loopStandalone() {
    if (isConnectedToBackend) {
        webSocket.loop();
    }
    
    // Keep mic processing if needed (depending on implementation, mic might be ISR or task based)
    // mic_process(); // Uncomment if mic.h requires polling
}

#endif // STANDALONE_H
