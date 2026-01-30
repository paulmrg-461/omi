#ifndef WIFI_UPLOADER_H
#define WIFI_UPLOADER_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "config.h"

void setupWiFi() {
    Serial.println("Connecting to WiFi...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    
    // We don't wait here to avoid blocking startup, 
    // but we check status before sending.
}

bool sendAudioToAPI(uint8_t* data, size_t len) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected, attempting reconnection...");
        WiFi.begin(WIFI_SSID, WIFI_PASS);
        int retries = 0;
        while (WiFi.status() != WL_CONNECTED && retries < 20) {
            delay(500);
            Serial.print(".");
            retries++;
        }
        Serial.println();
        
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("Failed to connect to WiFi");
            return false;
        }
    }

    Serial.println("Sending audio to API...");
    HTTPClient http;
    http.begin(API_ENDPOINT);
    http.addHeader("Content-Type", "application/octet-stream");
    
    int httpResponseCode = http.POST(data, len);
    
    bool success = false;
    if (httpResponseCode > 0) {
        Serial.printf("HTTP Response code: %d\n", httpResponseCode);
        String response = http.getString();
        Serial.println(response);
        success = true;
    } else {
        Serial.printf("Error code: %d\n", httpResponseCode);
    }
    
    http.end();
    return success;
}

#endif
