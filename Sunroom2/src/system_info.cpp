#include <Arduino.h>
#include <WiFi.h>
#include <map>
#include "definitions.h"
#include "esp_timer.h"
#include "json.h"
#include "time_helpers.h"
#include "units.h"

std::map<String, String> collectSystemInfo() {
    std::map<String, String> info;

    // Keep global in sync
    FREE_HEAP = ESP.getFreeHeap();

    info["ChipId"] = String(CHIP_ID, HEX);
    info["ResetCounter"] = String(RESET_COUNTER);
    info["LastResetReason"] = String(LAST_RESET_REASON);
    info["InternalTemperature"] = String(cToF(INTERNAL_CHIP_TEMPERATURE), 2);
    info["CurrentTime"] = getLocalTimeString();
    info["Core"] = String(xPortGetCoreID());
    info["FreeHeap"] = String(FREE_HEAP);
    info["MinFreeHeap"] = String(ESP.getMinFreeHeap());
    info["HeapSize"] = String(ESP.getHeapSize());
    info["FreeSketchSpace"] = String(ESP.getFreeSketchSpace());
    info["SketchSize"] = String(ESP.getSketchSize());
    info["CpuFrequencyMHz"] = String(ESP.getCpuFreqMHz());
    info["UptimeSeconds"] = String(static_cast<long long>(esp_timer_get_time() / 1000000ULL));

    // Optional WiFi details when connected
    info["WiFiStatus"] = String(WiFi.status());
    if (WiFi.status() == WL_CONNECTED) {
        info["WiFiRSSI"] = String(WiFi.RSSI());
        info["IPAddress"] = WiFi.localIP().toString();
        info["SSID"] = WiFi.SSID();
    }

    return info;
}

String systemInfoJson() { return buildJson(collectSystemInfo()); }
