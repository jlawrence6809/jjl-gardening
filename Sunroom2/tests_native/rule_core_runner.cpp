#include <iostream>
#include <string>
#include <functional>
#include <vector>
#include <map>
#include <ArduinoJson.h>

#include "../src/rule_core.h"
#include "../src/sensor_value.h"

// ANSI color codes
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_CYAN    "\033[36m"

static int failures = 0;

static void log_info(const std::string &msg)
{
    std::cout << COLOR_CYAN << "[INFO] " << msg << COLOR_RESET << std::endl;
}

static void log_debug(const std::string &msg)
{
    std::cout << COLOR_YELLOW << "[DEBUG] " << msg << COLOR_RESET << std::endl;
}

static void log_success(const std::string &msg)
{
    std::cout << COLOR_GREEN << "[PASS] " << msg << COLOR_RESET << std::endl;
}

static void log_error(const std::string &msg)
{
    std::cerr << COLOR_RED << "[FAIL] " << msg << COLOR_RESET << std::endl;
}

static void check(bool cond, const char *msg)
{
    if (!cond)
    {
        log_error(msg);
        failures++;
    }
    else
    {
        log_success(msg);
    }
}

int main()
{
    log_info("Running rule_core native checks...");

    // Test 1: ["EQ", true, true] => 1.0
    {
        DynamicJsonDocument doc(256);
        deserializeJson(doc, "[\"EQ\", true, true]");
        RuleCoreEnv env{};
        env.tryReadSensor = [](const std::string &, SensorValue &){ return false; };
        env.getCurrentSeconds = [](){ return 0; };
        auto r = processRuleCore(doc, env);
        check(r.type == FLOAT_TYPE && std::abs(r.val - 1.0f) < 0.001f, "EQ true,true -> 1.0");
    }

    // Test 2: IF GT(temperature,20) then SET relay_0 1 else SET relay_0 0
    {
        float last = -1;
        DynamicJsonDocument doc(512);
        deserializeJson(doc, "[\"IF\", [\"GT\", \"temperature\", 20], [\"SET\", \"relay_0\", 1], [\"SET\", \"relay_0\", 0]]");
        RuleCoreEnv env{};
        env.tryReadSensor = [](const std::string &name, SensorValue &out){ if (name=="temperature"){ out=SensorValue(22.0f); return true;} return false; };
        env.tryGetActuator = [&](const std::string &name, std::function<void(float)> &setter){ if (name=="relay_0"){ setter = [&](float v){ last=v; }; return true;} return false; };
        env.getCurrentSeconds = [](){ return 0; };
        auto r = processRuleCore(doc, env);
        (void)r;
        check(std::abs(last - 1.0f) < 0.001f, "IF-SET with temperature sensor sets relay to 1");
    }

    // Test 3: AND(EQ true,true, NOT(EQ true,false)) => 1.0
    {
        DynamicJsonDocument doc(1024);
        auto eTop = deserializeJson(doc, "[\"AND\", [\"EQ\", true, true], [\"NOT\", [\"EQ\", true, false]]]");
        if (eTop) { log_error("deserialize AND(...): " + std::string(eTop.c_str())); failures++; }
        RuleCoreEnv env{};
        env.tryReadSensor = [](const std::string &, SensorValue &){ return false; };
        env.getCurrentSeconds = [](){ return 0; };
        // Debug sub-evals
        DynamicJsonDocument aDoc(512); deserializeJson(aDoc, "[\"EQ\", true, true]");
        DynamicJsonDocument bDoc(512); deserializeJson(bDoc, "[\"EQ\", true, false]");
        auto a = processRuleCore(aDoc, env);
        auto b = processRuleCore(bDoc, env);
        log_debug("EQ(true,true) => type=" + std::to_string(a.type) + " val=" + std::to_string(a.val));
        log_debug("EQ(true,false) => type=" + std::to_string(b.type) + " val=" + std::to_string(b.val));
        DynamicJsonDocument notDoc(512); deserializeJson(notDoc, "[\"NOT\", [\"EQ\", true, false]]");
        auto n = processRuleCore(notDoc, env);
        log_debug("NOT(EQ(true,false)) => type=" + std::to_string(n.type) + " err=" + std::to_string(n.errorCode) + " val=" + std::to_string(n.val));
        auto r = processRuleCore(doc, env);
        log_debug("AND(...) => type=" + std::to_string(r.type) + " err=" + std::to_string(r.errorCode) + " val=" + std::to_string(r.val));
        check(r.type == FLOAT_TYPE && std::abs(r.val - 1.0f) < 0.001f, "AND/NOT returns 1.0");
    }

    // Test 4: OR(false, true) => 1.0
    {
        DynamicJsonDocument doc(128);
        deserializeJson(doc, "[\"OR\", false, true]");
        RuleCoreEnv env{};
        auto r = processRuleCore(doc, env);
        check(r.type == FLOAT_TYPE && std::abs(r.val - 1.0f) < 0.001f, "OR(false,true) => 1.0");
    }

    // Test 5: NOT(true) => 0.0
    {
        DynamicJsonDocument doc(128);
        deserializeJson(doc, "[\"NOT\", true]");
        RuleCoreEnv env{};
        auto r = processRuleCore(doc, env);
        check(r.type == FLOAT_TYPE && std::abs(r.val - 0.0f) < 0.001f, "NOT(true) => 0.0");
    }

    // Test 6: GT(25.5, 20.0) => 1.0
    {
        DynamicJsonDocument doc(128);
        deserializeJson(doc, "[\"GT\", 25.5, 20.0]");
        RuleCoreEnv env{};
        auto r = processRuleCore(doc, env);
        check(r.type == FLOAT_TYPE && std::abs(r.val - 1.0f) < 0.001f, "GT(25.5, 20.0) => 1.0");
    }

    // Test 7: LT(15.0, 20.0) => 1.0
    {
        DynamicJsonDocument doc(128);
        deserializeJson(doc, "[\"LT\", 15.0, 20.0]");
        RuleCoreEnv env{};
        auto r = processRuleCore(doc, env);
        check(r.type == FLOAT_TYPE && std::abs(r.val - 1.0f) < 0.001f, "LT(15.0, 20.0) => 1.0");
    }

    // Test 8: GTE(20.0, 20.0) => 1.0
    {
        DynamicJsonDocument doc(128);
        deserializeJson(doc, "[\"GTE\", 20.0, 20.0]");
        RuleCoreEnv env{};
        auto r = processRuleCore(doc, env);
        check(r.type == FLOAT_TYPE && std::abs(r.val - 1.0f) < 0.001f, "GTE(20.0, 20.0) => 1.0");
    }

    // Test 9: LTE(19.5, 20.0) => 1.0
    {
        DynamicJsonDocument doc(128);
        deserializeJson(doc, "[\"LTE\", 19.5, 20.0]");
        RuleCoreEnv env{};
        auto r = processRuleCore(doc, env);
        check(r.type == FLOAT_TYPE && std::abs(r.val - 1.0f) < 0.001f, "LTE(19.5, 20.0) => 1.0");
    }

    // Test 10: GT(15.0, 20.0) => 0.0 (false case)
    {
        DynamicJsonDocument doc(128);
        deserializeJson(doc, "[\"GT\", 15.0, 20.0]");
        RuleCoreEnv env{};
        auto r = processRuleCore(doc, env);
        check(r.type == FLOAT_TYPE && std::abs(r.val - 0.0f) < 0.001f, "GT(15.0, 20.0) => 0.0");
    }

    // Test 11: Sensor reading with GT comparison
    {
        DynamicJsonDocument doc(256);
        deserializeJson(doc, "[\"GT\", \"temperature\", 22.0]");
        RuleCoreEnv env{};
        env.tryReadSensor = [](const std::string &name, SensorValue &out){ 
            if (name == "temperature") { out = SensorValue(25.3f); return true; } 
            return false; 
        };
        auto r = processRuleCore(doc, env);
        check(r.type == FLOAT_TYPE && std::abs(r.val - 1.0f) < 0.001f, "GT(temperature=25.3, 22.0) => 1.0");
    }

    // Test 12: Time literal parsing (@HH:MM:SS format) 
    {
        DynamicJsonDocument doc(256);
        deserializeJson(doc, "[\"GT\", \"currentTime\", \"@14:30:00\"]");
        RuleCoreEnv env{};
        env.getCurrentSeconds = [](){ return 15 * 3600 + 45 * 60; }; // 15:45:00
        // Note: parseTimeLiteral is now handled internally by the rule engine
        auto r = processRuleCore(doc, env);
        check(r.type == FLOAT_TYPE && std::abs(r.val - 1.0f) < 0.001f, "GT(currentTime=15:45, @14:30:00) => 1.0");
    }

    // Test 13: Complex nested expression: AND(GT(temp, 20), LT(humidity, 80))
    {
        DynamicJsonDocument doc(512);
        deserializeJson(doc, "[\"AND\", [\"GT\", \"temperature\", 20.0], [\"LT\", \"humidity\", 80.0]]");
        RuleCoreEnv env{};
        env.tryReadSensor = [](const std::string &name, SensorValue &out){ 
            if (name == "temperature") { out = SensorValue(23.5f); return true; }
            if (name == "humidity") { out = SensorValue(65.2f); return true; }
            return false; 
        };
        auto r = processRuleCore(doc, env);
        check(r.type == FLOAT_TYPE && std::abs(r.val - 1.0f) < 0.001f, "AND(GT(temp=23.5, 20), LT(humidity=65.2, 80)) => 1.0");
    }

    // Test 14: OR with one false condition
    {
        DynamicJsonDocument doc(512);
        deserializeJson(doc, "[\"OR\", [\"GT\", \"temperature\", 30.0], [\"LT\", \"humidity\", 80.0]]");
        RuleCoreEnv env{};
        env.tryReadSensor = [](const std::string &name, SensorValue &out){ 
            if (name == "temperature") { out = SensorValue(23.5f); return true; }
            if (name == "humidity") { out = SensorValue(65.2f); return true; }
            return false; 
        };
        auto r = processRuleCore(doc, env);
        check(r.type == FLOAT_TYPE && std::abs(r.val - 1.0f) < 0.001f, "OR(GT(temp=23.5, 30), LT(humidity=65.2, 80)) => 1.0");
    }

    // Test 15: IF-ELSE with SET operations
    {
        float relay_value = -1;
        DynamicJsonDocument doc(512);
        deserializeJson(doc, "[\"IF\", [\"GT\", \"temperature\", 25.0], [\"SET\", \"relay_0\", 1.0], [\"SET\", \"relay_0\", 0.0]]");
        RuleCoreEnv env{};
        env.tryReadSensor = [](const std::string &name, SensorValue &out){ 
            if (name == "temperature") { out = SensorValue(22.0f); return true; }
            return false; 
        };
        env.tryGetActuator = [&](const std::string &name, std::function<void(float)> &setter){ 
            if (name == "relay_0") { 
                setter = [&](float v){ relay_value = v; }; 
                return true; 
            } 
            return false; 
        };
        auto r = processRuleCore(doc, env);
        (void)r;
        check(std::abs(relay_value - 0.0f) < 0.001f, "IF(GT(temp=22, 25), SET(1), SET(0)) sets relay to 0");
    }

    // Test 16: Error case - unknown sensor
    {
        DynamicJsonDocument doc(256);
        deserializeJson(doc, "[\"GT\", \"unknown_sensor\", 20.0]");
        RuleCoreEnv env{};
        env.tryReadSensor = [](const std::string &, SensorValue &){ return false; };
        auto r = processRuleCore(doc, env);
        check(r.type == ERROR_TYPE && r.errorCode != 0, "GT(unknown_sensor, 20) returns error");
    }

    // Test 17: Time literal parsing - valid cases
    {
        log_info("Testing time literal parsing - valid cases");
        
        // Test 17a: "@00:00:00" => 0
        {
            DynamicJsonDocument doc(128);
            deserializeJson(doc, "\"@00:00:00\"");
            RuleCoreEnv env{};
            auto r = processRuleCore(doc, env);
            check(r.type == FLOAT_TYPE && std::abs(r.val - 0.0f) < 0.001f, "Time literal @00:00:00 => 0");
        }
        
        // Test 17b: "@14:30:00" => 52200 (14*3600 + 30*60 + 0)
        {
            DynamicJsonDocument doc(128);
            deserializeJson(doc, "\"@14:30:00\"");
            RuleCoreEnv env{};
            auto r = processRuleCore(doc, env);
            check(r.type == FLOAT_TYPE && std::abs(r.val - 52200.0f) < 0.001f, "Time literal @14:30:00 => 52200");
        }
        
        // Test 17c: "@23:59:59" => 86399 (23*3600 + 59*60 + 59)
        {
            DynamicJsonDocument doc(128);
            deserializeJson(doc, "\"@23:59:59\"");
            RuleCoreEnv env{};
            auto r = processRuleCore(doc, env);
            check(r.type == FLOAT_TYPE && std::abs(r.val - 86399.0f) < 0.001f, "Time literal @23:59:59 => 86399");
        }
        
        // Test 17d: "@12:00:00" => 43200 (12*3600)
        {
            DynamicJsonDocument doc(128);
            deserializeJson(doc, "\"@12:00:00\"");
            RuleCoreEnv env{};
            auto r = processRuleCore(doc, env);
            check(r.type == FLOAT_TYPE && std::abs(r.val - 43200.0f) < 0.001f, "Time literal @12:00:00 => 43200");
        }
        
        // Test 17e: "@01:01:01" => 3661 (1*3600 + 1*60 + 1)
        {
            DynamicJsonDocument doc(128);
            deserializeJson(doc, "\"@01:01:01\"");
            RuleCoreEnv env{};
            auto r = processRuleCore(doc, env);
            check(r.type == FLOAT_TYPE && std::abs(r.val - 3661.0f) < 0.001f, "Time literal @01:01:01 => 3661");
        }
    }

    // Test 18: Time literal parsing - invalid cases
    {
        log_info("Testing time literal parsing - invalid cases");
        
        // Test 18a: Wrong length (too short)
        {
            DynamicJsonDocument doc(128);
            deserializeJson(doc, "\"@14:30\"");
            RuleCoreEnv env{};
            auto r = processRuleCore(doc, env);
            check(r.type == ERROR_TYPE && r.errorCode == TIME_ERROR, "Invalid time @14:30 => TIME_ERROR");
        }
        
        // Test 18b: Wrong length (too long)
        {
            DynamicJsonDocument doc(128);
            deserializeJson(doc, "\"@14:30:000\"");
            RuleCoreEnv env{};
            auto r = processRuleCore(doc, env);
            check(r.type == ERROR_TYPE && r.errorCode == TIME_ERROR, "Invalid time @14:30:000 => TIME_ERROR");
        }
        
        // Test 18c: Missing @ prefix
        {
            DynamicJsonDocument doc(128);
            deserializeJson(doc, "\"14:30:00\"");
            RuleCoreEnv env{};
            auto r = processRuleCore(doc, env);
            check(r.type == ERROR_TYPE && r.errorCode == UNREC_STR_ERROR, "Invalid time 14:30:00 => UNREC_STR_ERROR");
        }
        
        // Test 18d: Wrong colon positions
        {
            DynamicJsonDocument doc(128);
            deserializeJson(doc, "\"@14-30-00\"");
            RuleCoreEnv env{};
            auto r = processRuleCore(doc, env);
            check(r.type == ERROR_TYPE && r.errorCode == TIME_ERROR, "Invalid time @14-30-00 => TIME_ERROR");
        }
        
        // Test 18e: Non-digit characters
        {
            DynamicJsonDocument doc(128);
            deserializeJson(doc, "\"@1a:30:00\"");
            RuleCoreEnv env{};
            auto r = processRuleCore(doc, env);
            check(r.type == ERROR_TYPE && r.errorCode == TIME_ERROR, "Invalid time @1a:30:00 => TIME_ERROR");
        }
        
        // Test 18f: Invalid hour (>23)
        {
            DynamicJsonDocument doc(128);
            deserializeJson(doc, "\"@25:30:00\"");
            RuleCoreEnv env{};
            auto r = processRuleCore(doc, env);
            check(r.type == ERROR_TYPE && r.errorCode == TIME_ERROR, "Invalid time @25:30:00 => TIME_ERROR");
        }
        
        // Test 18g: Invalid minute (>59)
        {
            DynamicJsonDocument doc(128);
            deserializeJson(doc, "\"@14:70:00\"");
            RuleCoreEnv env{};
            auto r = processRuleCore(doc, env);
            check(r.type == ERROR_TYPE && r.errorCode == TIME_ERROR, "Invalid time @14:70:00 => TIME_ERROR");
        }
        
        // Test 18h: Invalid second (>59)
        {
            DynamicJsonDocument doc(128);
            deserializeJson(doc, "\"@14:30:70\"");
            RuleCoreEnv env{};
            auto r = processRuleCore(doc, env);
            check(r.type == ERROR_TYPE && r.errorCode == TIME_ERROR, "Invalid time @14:30:70 => TIME_ERROR");
        }
    }

    // Test 19: Time comparisons using parsed literals
    {
        log_info("Testing time comparisons with literals");
        
        // Test 19a: GT comparison with time literals
        {
            DynamicJsonDocument doc(256);
            deserializeJson(doc, "[\"GT\", \"@15:30:00\", \"@14:30:00\"]");
            RuleCoreEnv env{};
            auto r = processRuleCore(doc, env);
            check(r.type == FLOAT_TYPE && std::abs(r.val - 1.0f) < 0.001f, "GT(@15:30:00, @14:30:00) => 1.0");
        }
        
        // Test 19b: LT comparison with time literals
        {
            DynamicJsonDocument doc(256);
            deserializeJson(doc, "[\"LT\", \"@12:00:00\", \"@18:00:00\"]");
            RuleCoreEnv env{};
            auto r = processRuleCore(doc, env);
            check(r.type == FLOAT_TYPE && std::abs(r.val - 1.0f) < 0.001f, "LT(@12:00:00, @18:00:00) => 1.0");
        }
        
        // Test 19c: EQ comparison with identical time literals
        {
            DynamicJsonDocument doc(256);
            deserializeJson(doc, "[\"EQ\", \"@12:00:00\", \"@12:00:00\"]");
            RuleCoreEnv env{};
            auto r = processRuleCore(doc, env);
            check(r.type == FLOAT_TYPE && std::abs(r.val - 1.0f) < 0.001f, "EQ(@12:00:00, @12:00:00) => 1.0");
        }
        
        // Test 19d: Compare currentTime with time literal
        {
            DynamicJsonDocument doc(256);
            deserializeJson(doc, "[\"GT\", \"currentTime\", \"@08:00:00\"]");
            RuleCoreEnv env{};
            env.getCurrentSeconds = [](){ return 10 * 3600 + 30 * 60; }; // 10:30:00
            auto r = processRuleCore(doc, env);
            check(r.type == FLOAT_TYPE && std::abs(r.val - 1.0f) < 0.001f, "GT(currentTime=10:30, @08:00:00) => 1.0");
        }
    }

    // Test 20: processRuleSet with multiple rules using unified actuator system
    {
        std::string rules[] = {
            "[\"GT\", \"temperature\", 20]",
            "[\"SET\", \"relay_0\", 1]",
            "[\"LT\", \"humidity\", 80]"
        };
        
        // Track actuator calls with unified system
        struct ActuatorCall { std::string name; float value; };
        std::vector<ActuatorCall> actuatorCalls;
        
        RuleCoreEnv env{};
                env.tryReadSensor = [](const std::string &name, SensorValue &out){ 
            if (name == "temperature") { out = SensorValue(25.0f); return true; }
            if (name == "humidity") { out = SensorValue(60.0f); return true; }
            return false;
        };
        env.tryGetActuator = [&](const std::string &name, std::function<void(float)> &setter){
            if (name.rfind("relay_", 0) == 0) { // Any relay
                setter = [&, name](float v){ actuatorCalls.push_back({name, v}); };
                return true;
            }
            return false;
        };
        
        processRuleSet(rules, 3, env);
        
        // Should have calls: relay_0(2), relay_1(2), relay_2(2) for initialization
        // Plus: relay_0(1.0) for GT result, relay_0(1) for SET, relay_2(1.0) for LT result
        bool foundGTResult = false;
        bool foundSETResult = false;
        bool foundLTResult = false;
        
        for (const auto& call : actuatorCalls) {
            if (call.name == "relay_0" && std::abs(call.value - 1.0f) < 0.001f) foundGTResult = true;
            if (call.name == "relay_0" && std::abs(call.value - 1.0f) < 0.001f) foundSETResult = true; // SET also calls relay_0 with 1
            if (call.name == "relay_2" && std::abs(call.value - 1.0f) < 0.001f) foundLTResult = true;
        }
        
        check(foundGTResult, "processRuleSet: Rule 0 (GT) sets relay_0 to 1.0");
        check(foundSETResult, "processRuleSet: Rule 1 (SET) calls relay_0 with 1.0");
        check(foundLTResult, "processRuleSet: Rule 2 (LT) sets relay_2 to 1.0");
    }

    // Test SensorValue variant system
    log_info("Testing SensorValue variant system...");
    
    // Test 1: Basic type construction and conversion
    {
        log_debug("Test 1: Basic type construction and conversion");
        
        SensorValue floatVal(25.5f);
        SensorValue intVal(42);
        SensorValue stringVal("connected");
        
        check(floatVal.asFloat() == 25.5f, "SensorValue float construction and conversion");
        check(intVal.asInt() == 42, "SensorValue int construction and conversion");
        check(std::string(stringVal.asString()) == "connected", "SensorValue string construction and conversion");
        
        // Test explicit float conversion
        float explicitFloat = floatVal.asFloat();
        check(explicitFloat == 25.5f, "SensorValue explicit float conversion");
    }
    
    // Test 2: Cross-type conversions
    {
        log_debug("Test 2: Cross-type conversions");
        
        SensorValue floatVal(25.5f);
        SensorValue intVal(42);
        SensorValue stringFloatVal("123.45");
        SensorValue stringIntVal("456");
        SensorValue badStringVal("not_a_number");
        
        // Float to int (should truncate)
        check(floatVal.asInt() == 25, "Float to int conversion (truncate)");
        
        // Int to float (should be exact)
        check(intVal.asFloat() == 42.0f, "Int to float conversion");
        
        // String to float (should parse)
        check(stringFloatVal.asFloat() == 123.45f, "String to float conversion (valid)");
        
        // Bad string to float (should return 0.0)
        check(badStringVal.asFloat() == 0.0f, "String to float conversion (invalid)");
        
        // String to int (should parse integer strings)
        check(stringIntVal.asInt() == 456, "String to int conversion");
        check(badStringVal.asInt() == 0, "Bad string to int conversion");
        
        // Float strings should parse and truncate with two-stage parsing
        check(stringFloatVal.asInt() == 123, "Float string '123.45' parses to int 123 (two-stage)");
        
        // Test strict parsing - should reject partial conversions
        SensorValue partialFloat("123.45abc");
        SensorValue partialInt("123abc");
        SensorValue whitespaceFloat("123.45 ");
        check(partialFloat.asFloat() == 0.0f, "Strict parsing rejects '123.45abc'");
        check(partialInt.asInt() == 0, "Strict parsing rejects '123abc'");
        check(whitespaceFloat.asFloat() == 0.0f, "Strict parsing rejects '123.45 '");
        
        // Test two-stage int parsing consistency 
        SensorValue directFloat(25.7f);
        SensorValue stringFloat("25.7");
        SensorValue stringNegFloat("-3.8");
        SensorValue stringZeroFloat("0.0");
        
        check(directFloat.asInt() == stringFloat.asInt(), "Consistency: SensorValue(25.7f).asInt() == SensorValue(\"25.7\").asInt()");
        check(stringFloat.asInt() == 25, "Two-stage parsing: \"25.7\" -> int 25");
        check(stringNegFloat.asInt() == -3, "Two-stage parsing: \"-3.8\" -> int -3");
        check(stringZeroFloat.asInt() == 0, "Two-stage parsing: \"0.0\" -> int 0");
    }
    
    // Test 3: Comparison operators
    {
        log_debug("Test 3: Comparison operators");
        
        SensorValue float1(25.5f);
        SensorValue float2(25.5f);
        SensorValue float3(30.0f);
        SensorValue int1(25);
        SensorValue string1("connected");
        SensorValue string2("connected");
        SensorValue string3("disconnected");
        
        // Float equality
        check(float1 == float2, "Float equality (same value)");
        check(!(float1 == float3), "Float inequality (different value)");
        
        // String equality
        check(string1 == string2, "String equality (same value)");
        check(!(string1 == string3), "String inequality (different value)");
        
        // Cross-type comparison (should use float conversion)
        check(!(float1 == int1), "Float vs int comparison (25.5 != 25)");
        
        // Numeric comparisons
        check(float1 > int1, "Float > int comparison (25.5 > 25)");
        check(!(int1 > float1), "Int < float comparison (25 < 25.5)");
    }
    
    // Test 4: Integration with rule engine
    {
        log_debug("Test 4: Integration with rule engine");
        
        // Create environment with variant sensors
        RuleCoreEnv env{};
        std::map<std::string, SensorValue> sensors = {
            {"temperature", SensorValue(25.5f)},
            {"count", SensorValue(42)},
            {"status", SensorValue("online")},
            {"error_code", SensorValue("404")} // String that can be parsed as number
        };
        
        env.tryReadSensor = [&sensors](const std::string &name, SensorValue &out) {
            auto it = sensors.find(name);
            if (it != sensors.end()) {
                out = it->second;
                return true;
            }
            return false;
        };
        
        // Test reading different sensor types through rule engine
        DynamicJsonDocument doc1(256);
        deserializeJson(doc1, "\"temperature\"");
        RuleReturn result1 = processRuleCore(doc1, env);
        check(result1.type == FLOAT_TYPE && result1.val == 25.5f, "Rule engine reads float sensor");
        
        DynamicJsonDocument doc2(256);
        deserializeJson(doc2, "\"count\"");
        RuleReturn result2 = processRuleCore(doc2, env);
        check(result2.type == FLOAT_TYPE && result2.val == 42.0f, "Rule engine reads int sensor (converted to float)");
        
        DynamicJsonDocument doc3(256);
        deserializeJson(doc3, "\"error_code\"");
        RuleReturn result3 = processRuleCore(doc3, env);
        check(result3.type == FLOAT_TYPE && result3.val == 404.0f, "Rule engine reads string sensor (parsed as float)");
        
        // Test comparison with variant sensors
        DynamicJsonDocument doc4(256);
        deserializeJson(doc4, "[\"GT\", \"temperature\", 25]");
        RuleReturn result4 = processRuleCore(doc4, env);
        check(result4.type == FLOAT_TYPE && result4.val == 1.0f, "Rule engine comparison with variant sensor");
        
        DynamicJsonDocument doc5(256);
        deserializeJson(doc5, "[\"EQ\", \"count\", 42]");
        RuleReturn result5 = processRuleCore(doc5, env);
        check(result5.type == FLOAT_TYPE && result5.val == 1.0f, "Rule engine equality with int sensor");
    }
    
    if (failures == 0)
    {
        log_success("All native rule_core checks passed (including variant system).");
        return 0;
    }
    log_error(std::to_string(failures) + " failures.");
    return 1;
}


