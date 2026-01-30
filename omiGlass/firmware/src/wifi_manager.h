#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>

static String _ssid = "";
static String _password = "";
static bool _wifi_connected = false;

void setupWiFi(String ssid, String password) {
    _ssid = ssid;
    _password = password;
    
    if (ssid.length() == 0) return;

    Serial.printf("Connecting to WiFi: %s\n", ssid.c_str());
    WiFi.begin(ssid.c_str(), password.c_str());
    
    // We don't block here, we let the loop check status
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
