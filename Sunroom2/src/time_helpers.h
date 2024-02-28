#include <Arduino.h>

#pragma once

void updateTimeLoop();
int normalizeTimeToStartTime(int minuteOfDay, int startTime);
String getLocalTimeString();
String formatTime(int minutes);
