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

    if (failures == 0)
    {
        log_success("All native rule_core checks passed.");
        return 0;
    }
    log_error(std::to_string(failures) + " failures.");
    return 1;
}


