#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "definitions.h"
#include "interval_timer.h"
#include "time.h"

static String WORLDTIME_API = "http://worldtimeapi.org/api/ip";
// Refresh the time every 24 hours
static Timer refreshTimer(24 * 60 * 60 * 1000, true);
static Timer initializeTimer(5 * 60 * 1000, true);
constexpr int MINUTES_IN_DAY = 24 * 60;

long RAW_OFFSET = 0;
long DST_OFFSET = 0;

bool TIME_IS_SET = false;
bool TIMEZONE_OFFSET_IS_SET = false;

/**
 * Query for the time from the internet
 */
void queryForTime() {
    Serial.println("Querying for time...");
    configTime(RAW_OFFSET, DST_OFFSET, "pool.ntp.org", "time.nist.gov");
}

void checkTimeIsSet() {
    if (TIME_IS_SET) {
        return;
    }
    if (time(nullptr)) {
        TIME_IS_SET = true;
        struct tm timeinfo;
        getLocalTime(&timeinfo);  // Fix: Pass the address of timeinfo
        String timeString = String(asctime(&timeinfo));
        Serial.println("\nTime is set: " + timeString);
    } else {
        queryForTime();
    }
}

/**
 * Use the worldtimeapi.org API to get the timezone offset based on the IP address
 */
void queryForTimezoneOffset() {
    Serial.println("Querying for timezone offset...");
    HTTPClient http;
    http.begin(WORLDTIME_API);
    int httpCode = http.GET();

    if (httpCode > 0) {
        String payload = http.getString();
        DynamicJsonDocument doc(1024);
        deserializeJson(doc, payload);

        String timezone = doc["timezone"];  // The timezone
        RAW_OFFSET = doc["raw_offset"];     // The raw offset in seconds
        DST_OFFSET = doc["dst_offset"];     // The daylight savings offset in seconds

        Serial.println("Timezone: " + timezone);
        Serial.println("Raw offset: " + String(RAW_OFFSET));
        Serial.println("DST offset: " + String(DST_OFFSET));
        TIMEZONE_OFFSET_IS_SET = true;
    } else {
        Serial.println("Error in HTTP request");
    }
    http.end();
}

/**
 * Updates the time from the internet
 */
void updateTimeLoop() {
    if (WiFi.getMode() == WIFI_AP || WiFi.status() != WL_CONNECTED) {
        // If in AP mode or disconnected, we can't get time from the internet
        return;
    }

    if (refreshTimer.isIntervalPassed()) {
        TIME_IS_SET = false;
    }

    if (!initializeTimer.isIntervalPassed()) {
        return;
    }

    // We only need to query for the timezone offset once
    if (!TIMEZONE_OFFSET_IS_SET) {
        queryForTimezoneOffset();
        if (TIMEZONE_OFFSET_IS_SET) {
            queryForTime();

            // Add a little time for happy path time fetch to complete
            delay(1000);
        }
        return;
    }

    checkTimeIsSet();
}

/**
 * Returns the current time as a string
 * passes 9 in the ms argument of getLocalTime so it isn't blocking (check out the implementation of
 * getLocalTime)
 */
String getLocalTimeString() {
    struct tm timeinfo;
    getLocalTime(&timeinfo, 9);
    return String(asctime(&timeinfo));
}
