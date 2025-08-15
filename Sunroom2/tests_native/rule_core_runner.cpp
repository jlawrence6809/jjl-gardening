#include <iostream>
#include <string>
#include <functional>
#include <ArduinoJson.h>

#include "../src/rule_core.h"

static int failures = 0;

static void check(bool cond, const char *msg)
{
    if (!cond)
    {
        std::cerr << "FAIL: " << msg << std::endl;
        failures++;
    }
    else
    {
        std::cout << "OK:   " << msg << std::endl;
    }
}

int main()
{
    std::cout << "Running rule_core native checks..." << std::endl;

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
        if (eTop) { std::cerr << "FAIL: deserialize AND(...): " << eTop.c_str() << "\n"; failures++; }
        RuleCoreEnv env{};
        env.tryReadSensor = [](const std::string &, float &){ return false; };
        env.getCurrentSeconds = [](){ return 0; };
        env.parseTimeLiteral = [](const std::string &){ return -1; };
        // Debug sub-evals
        DynamicJsonDocument aDoc(512); deserializeJson(aDoc, "[\"EQ\", true, true]");
        DynamicJsonDocument bDoc(512); deserializeJson(bDoc, "[\"EQ\", true, false]");
        auto a = processRuleCore(aDoc, env);
        auto b = processRuleCore(bDoc, env);
        std::cout << "EQ(true,true) => type=" << a.type << " val=" << a.val << "\n";
        std::cout << "EQ(true,false) => type=" << b.type << " val=" << b.val << "\n";
        DynamicJsonDocument notDoc(512); deserializeJson(notDoc, "[\"NOT\", [\"EQ\", true, false]]");
        auto n = processRuleCore(notDoc, env);
        std::cout << "NOT(EQ(true,false)) => type=" << n.type << " err=" << n.errorCode << " val=" << n.val << "\n";
        auto r = processRuleCore(doc, env);
        std::cout << "AND(...) => type=" << r.type << " err=" << r.errorCode << " val=" << r.val << "\n";
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
        std::cout << "All native rule_core checks passed." << std::endl;
        return 0;
    }
    std::cerr << failures << " failures." << std::endl;
    return 1;
}


