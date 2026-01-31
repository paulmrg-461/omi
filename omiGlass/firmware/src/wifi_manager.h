#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>

static String _ssid = "";
static String _password = "";
static bool _wifi_connected = false;
static bool _should_connect = false;

void setupWiFi(String ssid, String password) {
    _ssid = ssid;
    _password = password;
    
    if (ssid.length() > 0) {
        _should_connect = true;
        Serial.println("WiFi credentials received. Scheduled connection...");
    }
}

void handleWiFiConnection() {
    if (!_should_connect) return;
    
    _should_connect = false;
    Serial.printf("Connecting to WiFi: %s\n", _ssid.c_str());
    WiFi.mode(WIFI_STA); // Ensure Station mode
    WiFi.setSleep(false); // Disable WiFi power save to prevent BLE interference
    WiFi.begin(_ssid.c_str(), _password.c_str());
}

bool checkWiFiConnection() {
    if (WiFi.status() == WL_CONNECTED && !_wifi_connected) {
        _wifi_connected = true;
        Serial.print("WiFi Connected! IP: ");
        Serial.println(WiFi.localIP());
        return true;
    } else if (WiFi.status() != WL_CONNECTED && _wifi_connected) {
        _wifi_connected = false;
        Serial.println("WiFi Disconnected");
    }
    return _wifi_connected;
}

String getWiFiIP() {
    if (_wifi_connected) return WiFi.localIP().toString();
    return "0.0.0.0";
}

#endif
