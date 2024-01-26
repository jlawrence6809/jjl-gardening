#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiClient.h>
#include "definitions.h"
#include "preferences_helpers.h"
#include "interval_timer.h"

#define WIFI_CONNECTION_TIMEOUT 15000

static Timer timer(30000, false);

static String ADDRESS = "";

/**
 * Sets up the mDNS responder
 * This happens after wifi connection is established, or after AP mode is entered
 */
void mDnsSetup()
{
    if (MDNS.begin(ADDRESS))
    {
        Serial.println("MDNS responder started. Address: " + ADDRESS + ".local");
    }
}

/**
 * Attempts to connect to wifi, returns true if connected, false if not
 */
bool attemptWifiConnection()
{
    if (SSID == "" || PASSWORD == "")
    {
        Serial.println("No ssid or password found in NVS, entering AP mode");
        return false;
    }

    Serial.println("Connecting to wifi...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID.c_str(), PASSWORD.c_str());

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
        if (millis() - start > WIFI_CONNECTION_TIMEOUT)
        {
            Serial.println("Could not connect to wifi, entering AP mode");
            return false;
        }
    }
    Serial.println("Connected to wifi");
    return true;
}

/**
 * Attempts to connect to wifi, if it fails, enters AP mode
 */
void connectWifiOrEnterApMode()
{
    bool connectionSuccessful = attemptWifiConnection();
    if (!connectionSuccessful)
    {
        Serial.println("Entering AP mode");
        WiFi.mode(WIFI_AP);
        WiFi.softAP(ADDRESS);
    }
    mDnsSetup();
}

/**
 * Connects to wifi, returns true if connected, false if not
 * Checks every 30 seconds
 */
void wifiCheckInLoop()
{
    if (!timer.isIntervalPassed())
    {
        return;
    }
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
    // if wifi is down, try reconnecting every 30 seconds
    if (WiFi.status() == WL_CONNECTED)
    {
        return;
    }

    // if in ap mode, don't try to reconnect
    if (WiFi.getMode() == WIFI_AP)
    {
        return;
    }

    Serial.println("WiFi disconnected. Status: " + String(WiFi.status()));
    // connectWifiOrEnterApMode();
}

void wifiSetup()
{
    // CHIP_ID is uint64_t, get the last two bytes as hex characters
    String hex = String(CHIP_ID, HEX);
    String lastByes = hex.substring(hex.length() - 4);
    ADDRESS = "esp_" + lastByes;

    connectWifiOrEnterApMode();
}