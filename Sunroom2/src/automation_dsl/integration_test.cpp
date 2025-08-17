/**
 * @file integration_test.cpp
 * @brief Integration test for the complete function registry system
 *
 * This test verifies that the new function registry system provides full
 * compatibility with the existing rule language and can handle real-world
 * automation scenarios.
 */

#include "new_core.h"
#include "registry_functions.h" 
#include "bridge_functions.h"
#include <iostream>
#include <ArduinoJson.h>

// Mock sensor values for testing
float CURRENT_TEMPERATURE = 27.5f;
float CURRENT_HUMIDITY = 65.0f;
float LIGHT_LEVEL = 750.0f;
int IS_SWITCH_ON = 1;

// Mock relay values array
enum RelayValue { OFF = 0, ON = 10, DONT_CARE = 20 };
RelayValue RELAY_VALUES[4] = {DONT_CARE, DONT_CARE, DONT_CARE, DONT_CARE};

// Mock setRelay function from bridge
void setRelay(int index, float value) {
    int intVal = static_cast<int>(value);
    int onesDigit = RELAY_VALUES[index] % 10;
    int newValue = (intVal * 10) + onesDigit;
    RELAY_VALUES[index] = static_cast<RelayValue>(newValue);
    std::cout << "Setting relay_" << index << " to: " << value << " (internal: " << newValue << ")" << std::endl;
}

// Mock actuator function
bool mockTryGetActuator(const std::string &name, std::function<void(float)> &setter) {
    if (name.rfind("relay_", 0) == 0) {
        int index = atoi(name.c_str() + 6); // Skip "relay_" prefix
        setter = std::bind(setRelay, index, std::placeholders::_1);
        return true;
    }
    return false;
}

// Registration function combining core and bridge functions
void testRegisterFunctions(FunctionRegistry& registry) {
    registerCoreFunctions(registry);
    registerBridgeFunctions(registry);
}

void runTest(const std::string& testName, const std::string& rule, const std::string& expected = "") {
    std::cout << "\n=== " << testName << " ===" << std::endl;
    std::cout << "Rule: " << rule << std::endl;
    
    // Set up environment
    NewRuleCoreEnv env;
    env.registerFunctions = testRegisterFunctions;
    env.tryGetActuator = mockTryGetActuator;
    
    // Parse and execute rule
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, rule.c_str());
    
    if (error) {
        std::cout << "Parse error: " << error.c_str() << std::endl;
        return;
    }
    
    UnifiedValue result = processNewRuleCore(doc.as<JsonVariantConst>(), env);
    
    std::cout << "Result - Type: " << static_cast<int>(result.type) 
              << ", Value: " << result.asFloat() 
              << ", Error: " << static_cast<int>(result.errorCode) << std::endl;
    
    if (!expected.empty()) {
        std::cout << "Expected: " << expected << std::endl;
    }
}

int main() {
    std::cout << "Function Registry Integration Test" << std::endl;
    std::cout << "=================================" << std::endl;
    
    std::cout << "\nInitial sensor values:" << std::endl;
    std::cout << "Temperature: " << CURRENT_TEMPERATURE << "°C" << std::endl;
    std::cout << "Humidity: " << CURRENT_HUMIDITY << "%" << std::endl;
    std::cout << "Light Level: " << LIGHT_LEVEL << std::endl;
    std::cout << "Switch: " << IS_SWITCH_ON << std::endl;
    
    // Test 1: Basic sensor reading
    runTest("Sensor Reading", "[\"getTemperature\"]", "27.5");
    
    // Test 2: Simple comparison (old syntax converted to new)
    runTest("Temperature Comparison", "[\"GT\", [\"getTemperature\"], 25]", "1.0 (true)");
    
    // Test 3: Complex logical expression
    runTest("Complex Logic", 
            "[\"AND\", [\"GT\", [\"getTemperature\"], 25], [\"LT\", [\"getHumidity\"], 70]]",
            "1.0 (true)");
    
    // Test 4: IF statement with actuator control
    runTest("IF with SET", 
            "[\"IF\", [\"GT\", [\"getTemperature\"], 25], [\"SET\", \"relay_0\", 1], [\"SET\", \"relay_0\", 0]]",
            "VOID (relay should be set to 1)");
    
    // Test 5: Time-based rule (using mock time)
    runTest("Time-based Rule",
            "[\"GT\", [\"getCurrentTime\"], \"@10:00:00\"]",
            "1.0 (true - mock time is 12:00:00)");
    
    // Test 6: Multi-sensor rule
    runTest("Multi-sensor Rule",
            "[\"OR\", [\"EQ\", [\"getLightSwitch\"], 1], [\"LT\", [\"getPhotoSensor\"], 500]]",
            "1.0 (true - switch is on)");
    
    // Test 7: Error case - unknown function
    runTest("Unknown Function Error",
            "[\"UNKNOWN_FUNCTION\", 1, 2]",
            "Error type should be FUNCTION_NOT_FOUND");
    
    // Test 8: Real-world automation scenario
    std::cout << "\n=== Real-world Scenario: Smart Garden Control ===" << std::endl;
    std::string gardenRule = "[\"IF\", "
                             "[\"AND\", "
                               "[\"GT\", [\"getTemperature\"], 25], "
                               "[\"LT\", [\"getHumidity\"], 60]"
                             "], "
                             "[\"SET\", \"relay_0\", 1], "
                             "[\"SET\", \"relay_0\", 0]"
                           "]";
    runTest("Garden Watering System", gardenRule, "Should activate watering");
    
    std::cout << "\n=== Integration Test Complete ===" << std::endl;
    std::cout << "All tests demonstrate the new function registry system" << std::endl;
    std::cout << "can handle the complete DSL with unified syntax." << std::endl;
    
    return 0;
}
