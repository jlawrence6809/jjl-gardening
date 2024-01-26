#include <Arduino.h>
#include <WiFi.h>
#include "time.h"
#include "definitions.h"
#include "interval_timer.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>

static String WORLDTIME_API = "http://worldtimeapi.org/api/ip";
static Timer timer(5 * 60 * 1000);
constexpr int MINUTES_IN_DAY = 24 * 60;

/**
 * Query for the time from the internet
 */
void queryForTime(long raw_offset, long dst_offset)
{
    Serial.println("Querying for time...");
    configTime(raw_offset, dst_offset, "pool.ntp.org", "time.nist.gov");
    while (!time(nullptr))
    {
        Serial.print(".");
        delay(1000);
    }
    struct tm timeinfo;
    getLocalTime(&timeinfo); // Fix: Pass the address of timeinfo
    String timeString = String(asctime(&timeinfo));
    Serial.println("\nTime is set: " + timeString);
}

/**
 * Use the worldtimeapi.org API to get the timezone offset based on the IP address
 */
void queryForTimezoneOffset(long &raw_offset, long &dst_offset)
{
    Serial.println("Querying for timezone offset...");
    HTTPClient http;
    http.begin(WORLDTIME_API);
    int httpCode = http.GET();

    if (httpCode > 0)
    {
        String payload = http.getString();
        DynamicJsonDocument doc(1024);
        deserializeJson(doc, payload);

        String timezone = doc["timezone"]; // The timezone
        raw_offset = doc["raw_offset"];    // The raw offset in seconds
        dst_offset = doc["dst_offset"];    // The daylight savings offset in seconds

        Serial.println("Timezone: " + timezone);
        Serial.println("Raw offset: " + String(raw_offset));
        Serial.println("DST offset: " + String(dst_offset));
    }
    else
    {
        Serial.println("Error in HTTP request");
    }
    http.end();
}

/**
 * Returns the positive modulo of a number
 */
int mod(int a, int b)
{
    int r = a % b;
    return r < 0 ? r + b : r;
}

/**
 * Returns the minuteOfDay argument normalized to the startTime argument being set to 0
 */
int normalizeTimeToStartTime(int minuteOfDay, int startTime)
{
    return mod(minuteOfDay - startTime, MINUTES_IN_DAY);
}

/**
 * Updates the time from the internet
 */
void updateTimeLoop()
{
    if (!timer.isIntervalPassed())
    {
        return;
    }

    if (WiFi.getMode() == WIFI_AP || WiFi.status() != WL_CONNECTED)
    {
        // If in AP mode or disconnected, we can't get time from the internet
        return;
    }

    long raw_offset = 0;
    long dst_offset = 0;
    queryForTimezoneOffset(
        raw_offset,
        dst_offset);
    queryForTime(
        raw_offset,
        dst_offset);
}