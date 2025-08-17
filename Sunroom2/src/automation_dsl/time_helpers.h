#pragma once

#include <string>

/**
 * @file time_helpers.h
 * @brief Time literal parsing utilities for the automation DSL
 *
 * This module provides utilities for parsing time literals in the "@HH:MM:SS" format
 * used by the automation rule engine. Time literals allow rules to compare against
 * specific times of day.
 *
 * Example usage in rules:
 * - ["GT", "currentTime", "@18:00:00"] - true if after 6 PM
 * - ["LT", "currentTime", "@08:30:00"] - true if before 8:30 AM
 */

/**
 * @brief Check if a string is a time literal
 * @param s The string to check
 * @return true if the string starts with '@', indicating a time literal
 *
 * Time literals are in the format "@HH:MM:SS" (e.g., "@14:30:00").
 * They are used in rules to compare against current time.
 *
 * Examples:
 * - jsonIsTimeLiteral("@14:30:00") returns true
 * - jsonIsTimeLiteral("temperature") returns false
 * - jsonIsTimeLiteral("") returns false
 */
bool jsonIsTimeLiteral(const std::string& s);

/**
 * @brief Parse a time literal string into seconds since midnight
 * @param timeStr Time string in "@HH:MM:SS" format (e.g., "@14:30:00")
 * @return Time in seconds since midnight, or -1 if parsing failed
 *
 * Platform-neutral implementation that only uses standard C++ libraries.
 * Used to parse time literals in rules like ["GT", "currentTime", "@18:00:00"]
 *
 * Examples:
 * - parseTimeLiteral("@14:30:00") returns (14*3600 + 30*60 + 0) = 52200
 * - parseTimeLiteral("@00:00:00") returns 0 (midnight)
 * - parseTimeLiteral("@23:59:59") returns 86399 (one second before midnight)
 * - parseTimeLiteral("invalid") returns -1
 * - parseTimeLiteral("@25:00:00") returns -1 (invalid hour)
 */
int parseTimeLiteral(const std::string& timeStr);
