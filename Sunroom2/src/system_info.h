#pragma once

#include <Arduino.h>
#include <map>

// Returns a key/value map of system information suitable for JSON serialization
std::map<String, String> collectSystemInfo();

// Convenience: builds JSON string from system info
String systemInfoJson();


