#include <Arduino.h>

#include "interval_timer.h"

Timer::Timer(unsigned long interval, bool runOnStart) {
    checkInterval = interval;
    if (runOnStart) {
        lastChecked = millis() - 2 * interval;
    } else {
        lastChecked = millis();
    }
}

bool Timer::isIntervalPassed() {
    if (millis() - lastChecked >= checkInterval) {
        lastChecked = millis();
        return true;
    }
    return false;
}
