#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiClient.h>
#include "definitions.h"
#include "preferences_helpers.h"
#include "interval_timer.h"

#define WIFI_CONNECTION_TIMEOUT 15000

/*
    WL_NO_SHIELD        = 255,   // for compatibility with WiFi Shield library
    WL_IDLE_STATUS      = 0,
    WL_NO_SSID_AVAIL    = 1,
    WL_SCAN_COMPLETED   = 2,
    WL_CONNECTED        = 3,
    WL_CONNECT_FAILED   = 4,
    WL_CONNECTION_LOST  = 5,
    WL_DISCONNECTED     = 6
*/

static Timer timer(30000, false);

/**
 * Connects to wifi, returns true if connected, false if not
 * Checks every 30 seconds
 */
void wifiCheckInLoop()
{
    // if wifi is down, try reconnecting every 30 seconds
    if (!timer.isIntervalPassed() || WiFi.status() == WL_CONNECTED)
    {
        return;
    }

    // Check that SSID is not empty
    if (SSID.length() == 0)
    {
        return;
    }

    WiFi.begin(SSID.c_str(), PASSWORD.c_str());
}

void wifiSetup()
{
    Serial.println("Setting up wifi...");
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(String(WIFI_NAME), String(AP_PASSWORD));
    if (SSID.length() > 0)
    {
        WiFi.begin(SSID.c_str(), PASSWORD.c_str());
        Serial.println("Connecting to wifi...");
    }
    if (MDNS.begin(WIFI_NAME))
    {
        Serial.println("MDNS responder started. Address: " + String(WIFI_NAME) + ".local");
    }
}