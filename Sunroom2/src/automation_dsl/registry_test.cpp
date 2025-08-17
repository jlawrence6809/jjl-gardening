/**
 * @file registry_test.cpp
 * @brief Simple test file for function registry system
 *
 * This is a standalone test file to verify the basic function registry
 * system works correctly. It can be compiled and run independently.
 */

#include "new_core.h"
#include "registry_functions.h"
#include <iostream>
#include <ArduinoJson.h>

// Mock sensor functions for testing
void registerTestSensorFunctions(FunctionRegistry& registry) {
    registry["getTemperature"] = [](JsonArrayConst args, const NewRuleCoreEnv& env) {
        return UnifiedValue(25.5f);  // Mock temperature
    };
    
    registry["getHumidity"] = [](JsonArrayConst args, const NewRuleCoreEnv& env) {
        return UnifiedValue(60.0f);  // Mock humidity
    };
}

// Mock actuator function
bool mockTryGetActuator(const std::string &name, std::function<void(float)> &outSetter) {
    if (name == "relay_0") {
        outSetter = [](float val) {
            std::cout << "Setting relay_0 to: " << val << std::endl;
        };
        return true;
    }
    return false;
}

int main() {
    std::cout << "Testing Function Registry System..." << std::endl;
    
    // Set up environment
    NewRuleCoreEnv env;
    env.registerFunctions = [](FunctionRegistry& registry) {
        registerCoreFunctions(registry);
        registerTestSensorFunctions(registry);
    };
    env.tryGetActuator = mockTryGetActuator;
    
    // Test 1: Simple comparison
    std::cout << "\nTest 1: GT comparison" << std::endl;
    DynamicJsonDocument doc1(256);
    deserializeJson(doc1, "[\"GT\", [\"getTemperature\"], 20]");
    UnifiedValue result1 = processNewRuleCore(doc1.as<JsonVariantConst>(), env);
    std::cout << "Result: " << result1.asFloat() << " (expected: 1)" << std::endl;
    
    // Test 2: Logical AND
    std::cout << "\nTest 2: AND operation" << std::endl;
    DynamicJsonDocument doc2(512);
    deserializeJson(doc2, "[\"AND\", [\"GT\", [\"getTemperature\"], 20], [\"LT\", [\"getHumidity\"], 70]]");
    UnifiedValue result2 = processNewRuleCore(doc2.as<JsonVariantConst>(), env);
    std::cout << "Result: " << result2.asFloat() << " (expected: 1)" << std::endl;
    
    // Test 3: IF statement with SET
    std::cout << "\nTest 3: IF with SET" << std::endl;
    DynamicJsonDocument doc3(512);
    deserializeJson(doc3, "[\"IF\", [\"GT\", [\"getTemperature\"], 20], [\"SET\", \"relay_0\", 1], [\"SET\", \"relay_0\", 0]]");
    UnifiedValue result3 = processNewRuleCore(doc3.as<JsonVariantConst>(), env);
    std::cout << "Result type: " << static_cast<int>(result3.type) << " (expected: VOID_TYPE = 2)" << std::endl;
    
    // Test 4: Function not found error
    std::cout << "\nTest 4: Function not found" << std::endl;
    DynamicJsonDocument doc4(256);
    deserializeJson(doc4, "[\"UNKNOWN_FUNCTION\", 1, 2]");
    UnifiedValue result4 = processNewRuleCore(doc4.as<JsonVariantConst>(), env);
    std::cout << "Error type: " << static_cast<int>(result4.errorCode) << " (expected: FUNCTION_NOT_FOUND)" << std::endl;
    
    std::cout << "\nTesting complete!" << std::endl;
    return 0;
}
