#pragma once

#include <ArduinoJson.h>
#include <functional>
#include <string>

#include "rule_helpers.h" // Reuse TypeCode, ErrorCode, RuleReturn

struct RuleCoreEnv {
    // Return true and set outVal if sensor is known; false if unknown
    std::function<bool(const std::string &name, float &outVal)> tryReadSensor;
    // Return true and set outSetter if actuator is known; false if unknown
    std::function<bool(const std::string &name, std::function<void(float)> &outSetter)> tryGetActuator;
    // Current time in seconds
    std::function<int()> getCurrentSeconds;
    // Parse time literal like "@HH:MM:SS"; return -1 on error
    std::function<int(const std::string &hhmmss)> parseTimeLiteral;
};

RuleReturn processRuleCore(JsonVariantConst doc, const RuleCoreEnv &env);

// Process a set of rules with generic result handling
void processRuleSet(
    const std::string rules[], 
    int ruleCount, 
    const RuleCoreEnv &env
);


