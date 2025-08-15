#include <iostream>
#include <string>
#include <functional>
#include <ArduinoJson.h>

#include "../src/rule_core.h"

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
        env.tryReadSensor = [](const std::string &, float &){ return false; };
        env.getCurrentSeconds = [](){ return 0; };
        env.parseTimeLiteral = [](const std::string &){ return -1; };
        auto r = processRuleCore(doc, env);
        check(r.type == FLOAT_TYPE && std::abs(r.val - 1.0f) < 0.001f, "EQ true,true -> 1.0");
    }

    // Test 2: IF GT(temperature,20) then SET relay_0 1 else SET relay_0 0
    {
        float last = -1;
        DynamicJsonDocument doc(512);
        deserializeJson(doc, "[\"IF\", [\"GT\", \"temperature\", 20], [\"SET\", \"relay_0\", 1], [\"SET\", \"relay_0\", 0]]");
        RuleCoreEnv env{};
        env.tryReadSensor = [](const std::string &name, float &out){ if (name=="temperature"){ out=22.0f; return true;} return false; };
        env.tryGetActuator = [&](const std::string &name, std::function<void(float)> &setter){ if (name=="relay_0"){ setter = [&](float v){ last=v; }; return true;} return false; };
        env.getCurrentSeconds = [](){ return 0; };
        env.parseTimeLiteral = [](const std::string &){ return -1; };
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
        env.tryReadSensor = [](const std::string &, float &){ return false; };
        env.getCurrentSeconds = [](){ return 0; };
        env.parseTimeLiteral = [](const std::string &){ return -1; };
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
        env.tryReadSensor = [](const std::string &name, float &out){ 
            if (name == "temperature") { out = 25.3f; return true; } 
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
        env.parseTimeLiteral = [](const std::string &timeStr){ 
            if (timeStr == "@14:30:00") return 14 * 3600 + 30 * 60; // 14:30:00
            return -1;
        };
        auto r = processRuleCore(doc, env);
        check(r.type == FLOAT_TYPE && std::abs(r.val - 1.0f) < 0.001f, "GT(currentTime=15:45, @14:30:00) => 1.0");
    }

    // Test 13: Complex nested expression: AND(GT(temp, 20), LT(humidity, 80))
    {
        DynamicJsonDocument doc(512);
        deserializeJson(doc, "[\"AND\", [\"GT\", \"temperature\", 20.0], [\"LT\", \"humidity\", 80.0]]");
        RuleCoreEnv env{};
        env.tryReadSensor = [](const std::string &name, float &out){ 
            if (name == "temperature") { out = 23.5f; return true; }
            if (name == "humidity") { out = 65.2f; return true; }
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
        env.tryReadSensor = [](const std::string &name, float &out){ 
            if (name == "temperature") { out = 23.5f; return true; }
            if (name == "humidity") { out = 65.2f; return true; }
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
        env.tryReadSensor = [](const std::string &name, float &out){ 
            if (name == "temperature") { out = 22.0f; return true; }
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
        env.tryReadSensor = [](const std::string &, float &){ return false; };
        auto r = processRuleCore(doc, env);
        check(r.type == ERROR_TYPE && r.errorCode != 0, "GT(unknown_sensor, 20) returns error");
    }

    if (failures == 0)
    {
        log_success("All native rule_core checks passed.");
        return 0;
    }
    log_error(std::to_string(failures) + " failures.");
    return 1;
}


