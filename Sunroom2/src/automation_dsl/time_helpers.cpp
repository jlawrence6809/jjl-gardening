/**
 * @file time_helpers.cpp
 * @brief Time literal parsing utilities implementation
 *
 * This file implements platform-neutral time literal parsing for the automation DSL.
 * Time literals allow rules to compare against specific times of day using the
 * "@HH:MM:SS" format.
 */

#include "time_helpers.h"
#include <string>

bool jsonIsTimeLiteral(const std::string& s) { return !s.empty() && s[0] == '@'; }

int parseTimeLiteral(const std::string& timeStr) {
    // Validate basic format: @HH:MM:SS (9 characters)
    if (timeStr.length() != 9) {
        return -1;
    }

    // Check format: must start with '@' and have colons at positions 3 and 6
    if (timeStr[0] != '@' || timeStr[3] != ':' || timeStr[6] != ':') {
        return -1;
    }

    // Extract and validate hour, minute, second strings
    std::string hourStr = timeStr.substr(1, 2);
    std::string minuteStr = timeStr.substr(4, 2);
    std::string secondStr = timeStr.substr(7, 2);

    // Check that all characters are digits
    for (char c : hourStr)
        if (c < '0' || c > '9') return -1;
    for (char c : minuteStr)
        if (c < '0' || c > '9') return -1;
    for (char c : secondStr)
        if (c < '0' || c > '9') return -1;

    // Convert to integers
    int hours = std::stoi(hourStr);
    int minutes = std::stoi(minuteStr);
    int seconds = std::stoi(secondStr);

    // Validate ranges
    if (hours < 0 || hours > 23) return -1;
    if (minutes < 0 || minutes > 59) return -1;
    if (seconds < 0 || seconds > 59) return -1;

    // Convert to seconds since midnight
    return (hours * 3600) + (minutes * 60) + seconds;
}
