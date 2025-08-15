#include <Arduino.h>
#include "definitions.h"
#include "interval_timer.h"
#include <WiFi.h>
#include "system_info.h"

// Run every 10 seconds
static Timer timer(10010);


void systemStatusLoop(){
    if(!timer.isIntervalPassed()){
        return;
    }
    
    Serial.println("=== System Status ===");
    
    // Ensure FREE_HEAP is updated consistently
    FREE_HEAP = ESP.getFreeHeap();
    
    auto info = collectSystemInfo();
    for (const auto &entry : info)
    {
        Serial.print(entry.first);
        Serial.print(": ");
        Serial.println(entry.second);
    }
    Serial.println("=====================");
}
