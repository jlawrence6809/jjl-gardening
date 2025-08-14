#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiClient.h>
#include "definitions.h"
#include "preferences_helpers.h"
#include "interval_timer.h"

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
static bool mdnsStarted = false;

// Backward compatibility for core versions using SYSTEM_EVENT_* names
#ifndef ARDUINO_EVENT_WIFI_STA_GOT_IP
#define ARDUINO_EVENT_WIFI_STA_GOT_IP SYSTEM_EVENT_STA_GOT_IP
#define ARDUINO_EVENT_WIFI_STA_DISCONNECTED SYSTEM_EVENT_STA_DISCONNECTED
#endif

/**
 * Connects to wifi, returns true if connected, false if not
 * Checks every 30 seconds
 */
bool wifiCheckInLoop()
{
    // if wifi is down, try reconnecting every 30 seconds
    wl_status_t wifiStatus = WiFi.status();
    if (!timer.isIntervalPassed() || wifiStatus == WL_CONNECTED)
    {
        return wifiStatus == WL_CONNECTED;
    }

    // Check that SSID is not empty
    if (SSID.length() == 0)
    {
        return false;
    }

    // Gentle nudge: ask WiFi stack to reconnect without re-writing credentials
    Serial.println("WiFi not connected; attempting reconnect()");
    WiFi.reconnect();
    return false;
}

void setupAP()
{
    // Start AP; require password of at least 8 chars, else open AP
    bool apOk = false;
    const int apChannel = 11; // pick a common 2.4GHz channel (1/6/11)
    const int hidden = 0;
    const int maxConn = 8;
    if (strlen(AP_PASSWORD) >= 8)
    {
        Serial.println("Starting AP with password");
        apOk = WiFi.softAP(WIFI_NAME, AP_PASSWORD, apChannel, hidden, maxConn);
    }
    else
    {
        Serial.println("AP password too short; starting open AP");
        apOk = WiFi.softAP(WIFI_NAME, nullptr, apChannel, hidden, maxConn);
    }
    if (!apOk)
    {
        Serial.println("softAP failed to start");
    }
    else
    {
        Serial.print("AP IP: ");
        Serial.println(WiFi.softAPIP());
    }
}

void wifiSetup()
{
    Serial.println("Setting up wifi...");
    WiFi.mode(WIFI_AP_STA);

    // Avoid writing credentials to flash repeatedly, set persistent to false to avoid writing to flash
    WiFi.persistent(false);

    // Set hostname and enable auto-reconnect
    WiFi.setHostname(WIFI_NAME);
    WiFi.setAutoReconnect(true);

    setupAP();

    // Start mDNS immediately for AP clients; add HTTP service advertisement
    if (!mdnsStarted)
    {
        if (MDNS.begin(WIFI_NAME))
        {
            mdnsStarted = true;
            Serial.println("MDNS responder started. Address: " + String(WIFI_NAME) + ".local");
            MDNS.addService("http", "tcp", 80);
            MDNS.addServiceTxt("http", "tcp", "path", "/");
        }
        else
        {
            Serial.println("MDNS start failed");
        }
    }

    // Register WiFi event handlers for visibility and mDNS setup
    WiFi.onEvent([](WiFiEvent_t event) {
        switch (event)
        {
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            Serial.print("Got IP: ");
            Serial.println(WiFi.localIP());
            // Ensure mDNS service is advertised even if it was started in AP mode
            if (mdnsStarted)
            {
                MDNS.addService("http", "tcp", 80);
                MDNS.addServiceTxt("http", "tcp", "path", "/");
            }
            break;
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            Serial.println("WiFi disconnected");
            break;
        default:
            Serial.println("WiFi event: " + String(event));
            break;
        }
    });

    // No initial scan; rely on auto-reconnect

    if (SSID.length() > 0)
    {
        WiFi.begin(SSID.c_str(), PASSWORD.c_str());
        Serial.println("Connecting to wifi...");
    }
}