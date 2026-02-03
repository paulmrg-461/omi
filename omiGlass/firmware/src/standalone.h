#ifndef STANDALONE_H
#define STANDALONE_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
// #include <WebSocketsClient.h> // Links2004/arduinoWebSockets (DISABLED FOR PHOTO MODE)
#include <ArduinoJson.h>
#include "config.h"
#include "mic.h"
#include "esp_camera.h" // Ensure camera access

// Global objects
// WebSocketsClient webSocket; // DISABLED
String deviceId = "";
bool isConnectedToBackend = false;

// WebSocket event handler (DISABLED)
/*
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
    switch(type) {
        case WStype_DISCONNECTED:
            Serial.println("[WSc] Disconnected!");
            isConnectedToBackend = false;
            break;
        case WStype_CONNECTED:
            Serial.printf("[WSc] Connected to url: %s\n", payload);
            isConnectedToBackend = true;
            break;
        case WStype_TEXT:
            Serial.printf("[WSc] get text: %s\n", payload);
            break;
        case WStype_BIN:
            Serial.printf("[WSc] get binary length: %u\n", length);
            break;
        case WStype_ERROR:
            Serial.printf("[WSc] Error: %s\n", payload);
            break;
        case WStype_FRAGMENT_TEXT_START:
        case WStype_FRAGMENT_BIN_START:
        case WStype_FRAGMENT:
        case WStype_FRAGMENT_FIN:
            break;
    }
}
*/

// Audio callback (DISABLED)
void standaloneAudioCallback(int16_t *data, size_t samples) {
    // Audio streaming disabled
}

// Function to capture and send photo
void captureAndSendImage() {
    Serial.println("Capturing photo for analysis...");
    
    // Capture photo
    camera_fb_t * fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("Camera capture failed");
        return;
    }
    Serial.printf("Photo captured: %u bytes\n", fb->len);

    // Prepare connection
    WiFiClientSecure client;
    client.setInsecure(); // Skip verification for DevTunnels/Self-signed

    // Parse Host from API_BASE_URL
    String host = String(API_BASE_URL);
    host.replace("https://", "");
    host.replace("http://", "");
    if (host.endsWith("/")) host.remove(host.length()-1);
    
    // Default port 443 for HTTPS
    int port = 443; 

    Serial.println("Connecting to: " + host);
    
    if (client.connect(host.c_str(), port)) {
        Serial.println("Connected to server!");

        String boundary = "------------------------esp32cam";
        String head = "--" + boundary + "\r\nContent-Disposition: form-data; name=\"file\"; filename=\"capture.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
        String tail = "\r\n--" + boundary + "--\r\n";
        
        uint32_t totalLen = head.length() + fb->len + tail.length();
        
        // Send HTTP Headers
        client.println("POST /vision/analyze HTTP/1.1");
        client.println("Host: " + host);
        client.println("Content-Length: " + String(totalLen));
        client.println("Content-Type: multipart/form-data; boundary=" + boundary);
        client.println();
        
        // Send Body
        client.print(head);
        
        // Send Image Data in chunks
        uint8_t *fbBuf = fb->buf;
        size_t fbLen = fb->len;
        size_t bufferSize = 1024;
        for (size_t i = 0; i < fbLen; i += bufferSize) {
            size_t remaining = fbLen - i;
            if (remaining < bufferSize) bufferSize = remaining;
            client.write(fbBuf + i, bufferSize);
        }
        
        client.print(tail);
        
        // Read Response
        Serial.println("Waiting for response...");
        long start = millis();
        while (client.connected() && millis() - start < 20000) {
            if (client.available()) {
                String line = client.readStringUntil('\n');
                if (line == "\r") {
                    Serial.println("Headers received.");
                    break;
                }
            }
            delay(10);
        }
        
        // Read Body
        while (client.available()) {
             String line = client.readString();
             Serial.println(line);
        }
        
        client.stop();
        Serial.println("Transaction complete.");
    } else {
        Serial.println("Connection failed!");
    }
    
    esp_camera_fb_return(fb);
}

void setupStandalone() {
    Serial.println("----------------------------------------");
    Serial.println("Starting Standalone Mode (PHOTO MODE)");
    Serial.println("----------------------------------------");

    // 1. Initialize WiFi
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    // Scan for networks to debug NO_AP_FOUND
    Serial.println("Scanning for networks...");
    int n = WiFi.scanNetworks();
    Serial.println("Scan done");
    if (n == 0) {
        Serial.println("No networks found");
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
    
    delay(1000);
    WiFi.setTxPower(WIFI_POWER_19_5dBm);
    
    // Connect WiFi
    Serial.print("Connecting to WiFi: "); Serial.println(WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(1000);
        Serial.print(".");
        attempts++;
    }
    Serial.println("");

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("WiFi Connected!");
        Serial.print("IP Address: "); Serial.println(WiFi.localIP());
        
        // SYNC TIME for SSL
        configTime(0, 0, "pool.ntp.org", "time.nist.gov");
        Serial.print("Waiting for NTP time sync: ");
        time_t now = time(nullptr);
        while (now < 8 * 3600 * 2) { 
            delay(500);
            Serial.print(".");
            now = time(nullptr);
        }
        Serial.println("");
        
        // Register Device (Optional for Photo Mode but good for ID)
        if (deviceId == "") {
             WiFiClientSecure client;
             client.setInsecure(); 
             client.setTimeout(15000);

             HTTPClient http;
             String registerUrl = String(API_BASE_URL) + "/devices/connect";
             Serial.println("Registering device at: " + registerUrl);
             
             http.begin(client, registerUrl);
             http.addHeader("Content-Type", "application/json");
             
             StaticJsonDocument<200> doc;
             doc["mac_address"] = WiFi.macAddress();
             doc["model"] = "XIAO_ESP32S3";
             doc["firmware_version"] = "2.1.1";
             
             String payload;
             serializeJson(doc, payload);
             
             int httpResponseCode = http.POST(payload);
             if (httpResponseCode > 0) {
                 String response = http.getString();
                 Serial.println("HTTP Response: " + String(httpResponseCode));
                 Serial.println("Body: " + response);
                 
                 StaticJsonDocument<512> responseDoc;
                 DeserializationError error = deserializeJson(responseDoc, response);
                 if (!error && responseDoc.containsKey("id")) {
                     deviceId = responseDoc["id"].as<String>();
                     Serial.println("Device ID: " + deviceId);
                 }
             }
             http.end();
        }
        
        // WebSocket setup DISABLED
    }
}

void loopStandalone() {
    // webSocket.loop(); // DISABLED
    
    static unsigned long lastPhotoTime = 0;
    // Capture photo every 60 seconds
    if (millis() - lastPhotoTime > 60000) {
        lastPhotoTime = millis();
        if (WiFi.status() == WL_CONNECTED) {
            captureAndSendImage();
        } else {
            Serial.println("WiFi Disconnected. Performing full radio reset...");
            
            // Full WiFi Reset Sequence
            WiFi.disconnect(true);   // Disconnect and turn off WiFi
            WiFi.mode(WIFI_OFF);
            delay(500);
            
            WiFi.mode(WIFI_STA);
            WiFi.setTxPower(WIFI_POWER_19_5dBm);
            delay(100);
            
            Serial.println("Scanning for networks...");
            int n = WiFi.scanNetworks();
            Serial.println("Scan done");
            
            if (n == 0) {
                Serial.println("No networks found");
            } else if (n < 0) {
                Serial.printf("Scan failed with error: %d\n", n);
            } else {
                Serial.printf("%d networks found\n", n);
                for (int i = 0; i < n; ++i) {
                    Serial.printf("%d: %s (%d) %s\n", i+1, WiFi.SSID(i).c_str(), WiFi.RSSI(i), (WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?" ":"*");
                }
            }
            
            Serial.print("Attempting reconnect to ");
            Serial.println(WIFI_SSID);
            WiFi.begin(WIFI_SSID, WIFI_PASS);
            
            // Wait up to 10 seconds for connection
            unsigned long startAttempt = millis();
            while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 10000) {
                delay(500);
                Serial.print(".");
            }
            Serial.println();
            
            if (WiFi.status() == WL_CONNECTED) {
                 Serial.println("Reconnected successfully!");
                 Serial.print("IP: "); Serial.println(WiFi.localIP());
                 // Try to send image immediately after reconnect
                 captureAndSendImage();
            } else {
                 Serial.println("Reconnect failed.");
            }
        }
    }
}

#endif // STANDALONE_H
