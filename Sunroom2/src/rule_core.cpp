#include "rule_core.h"
#include <string>
#include <cstring>

static RuleReturn createRuleReturn(TypeCode type, ErrorCode errorCode, float val) {
    return {type, errorCode, val, nullptr};
}
static RuleReturn createErrorRuleReturn(ErrorCode error) {
    return createRuleReturn(ERROR_TYPE, error, 0.0f);
}
static RuleReturn createFloatRuleReturn(float v) {
    return createRuleReturn(FLOAT_TYPE, NO_ERROR, v);
}
static RuleReturn createIntRuleReturn(int v) {
    return createFloatRuleReturn(static_cast<float>(v));
}
static RuleReturn createVoidRuleReturn() {
    return createRuleReturn(VOID_TYPE, NO_ERROR, 0.0f);
}
static RuleReturn createBoolRuleReturn(bool b) {
    return createFloatRuleReturn(b ? 1.0f : 0.0f);
}

static bool jsonIsTimeLiteral(const std::string &s) { return !s.empty() && s[0] == '@'; }

RuleReturn processRuleCore(JsonVariantConst doc, const RuleCoreEnv &env)
{
    RuleReturn voidReturn = createVoidRuleReturn();

    if (!doc.is<JsonArrayConst>())
    {
        if (doc.is<const char*>())
        {
            const char *cstr = doc.as<const char*>();
            std::string str = cstr ? std::string(cstr) : std::string();
            if (jsonIsTimeLiteral(str))
            {
                int secs = env.parseTimeLiteral ? env.parseTimeLiteral(str) : -1;
                return secs < 0 ? createErrorRuleReturn(TIME_ERROR) : createIntRuleReturn(secs);
            }

            // actuator?
            if (env.tryGetActuator)
            {
                std::function<void(float)> setter;
                if (env.tryGetActuator(str, setter) && setter)
                {
                    RuleReturn r = createRuleReturn(BOOL_ACTUATOR_TYPE, NO_ERROR, 0.0f);
                    r.actuatorSetter = setter;
                    return r;
                }
            }

            // sensor?
            if (env.tryReadSensor)
            {
                float val = 0.0f;
                if (env.tryReadSensor(str, val))
                {
                    return createFloatRuleReturn(val);
                }
            }

            if (str == std::string("currentTime"))
            {
                int secs = env.getCurrentSeconds ? env.getCurrentSeconds() : -1;
                return secs < 0 ? createErrorRuleReturn(TIME_ERROR) : createIntRuleReturn(secs);
            }

            return createErrorRuleReturn(UNREC_STR_ERROR);
        }
        else if (doc.is<bool>())
        {
            return createBoolRuleReturn(doc.as<bool>());
        }
        else if (doc.is<int>())
        {
            return createIntRuleReturn(doc.as<int>());
        }
        else if (doc.is<float>())
        {
            return createFloatRuleReturn(doc.as<float>());
        }
        else
        {
            return createErrorRuleReturn(UNREC_TYPE_ERROR);
        }
    }

    JsonArrayConst array = doc.as<JsonArrayConst>();
    const char *type = array[0].as<const char*>();
    if (!type)
    {
        return createErrorRuleReturn(UNREC_FUNC_ERROR);
    }

    if (std::strcmp(type, "NOP") == 0)
    {
        return voidReturn;
    }
    else if (std::strcmp(type, "IF") == 0)
    {
        RuleReturn cond = processRuleCore(array[1], env);
        if (cond.type == ERROR_TYPE) return cond;
        if (cond.type != FLOAT_TYPE) return createErrorRuleReturn(IF_CONDITION_ERROR);
        return cond.val > 0 ? processRuleCore(array[2], env) : processRuleCore(array[3], env);
    }
    else if (std::strcmp(type, "SET") == 0)
    {
        RuleReturn act = processRuleCore(array[1], env);
        RuleReturn val = processRuleCore(array[2], env);
        if (act.type == ERROR_TYPE) return act;
        if (val.type == ERROR_TYPE) return val;
        if (act.type != BOOL_ACTUATOR_TYPE || val.type != FLOAT_TYPE) return createErrorRuleReturn(BOOL_ACTUATOR_ERROR);
        if (act.actuatorSetter) act.actuatorSetter(val.val);
        return voidReturn;
    }
    else if (std::strcmp(type, "AND") == 0 || std::strcmp(type, "OR") == 0)
    {
        // Short-circuit semantics
        RuleReturn a = processRuleCore(array[1], env);
        if (a.type == ERROR_TYPE) return a;
        if (std::strcmp(type, "AND") == 0 && !(a.val > 0)) return createBoolRuleReturn(false);
        if (std::strcmp(type, "OR") == 0 && (a.val > 0)) return createBoolRuleReturn(true);
        RuleReturn b = processRuleCore(array[2], env);
        if (a.type == ERROR_TYPE) return a;
        if (b.type == ERROR_TYPE) return b;
        if (a.type != FLOAT_TYPE || b.type != FLOAT_TYPE) return createErrorRuleReturn(AND_OR_ERROR);
        bool result = (std::strcmp(type, "AND") == 0) ? (a.val > 0 && b.val > 0) : (a.val > 0 || b.val > 0);
        return createBoolRuleReturn(result);
    }
    else if (std::strcmp(type, "NOT") == 0)
    {
        RuleReturn a = processRuleCore(array[1], env);
        if (a.type == ERROR_TYPE) return a;
        if (a.type != FLOAT_TYPE) return createErrorRuleReturn(NOT_ERROR);
        return createBoolRuleReturn(!(a.val > 0));
    }
    else if (std::strcmp(type, "EQ") == 0 || std::strcmp(type, "NE") == 0 || std::strcmp(type, "GT") == 0 || std::strcmp(type, "LT") == 0 || std::strcmp(type, "GTE") == 0 || std::strcmp(type, "LTE") == 0)
    {
        RuleReturn a = processRuleCore(array[1], env);
        RuleReturn b = processRuleCore(array[2], env);
        if (a.type == ERROR_TYPE) return a;
        if (b.type == ERROR_TYPE) return b;
        if (a.type != FLOAT_TYPE || b.type != FLOAT_TYPE) return createErrorRuleReturn(COMPARISON_TYPE_EQUALITY_ERROR);
        bool res = false;
        if (std::strcmp(type, "EQ") == 0) res = a.val == b.val;
        else if (std::strcmp(type, "NE") == 0) res = a.val != b.val;
        else if (std::strcmp(type, "GT") == 0) res = a.val > b.val;
        else if (std::strcmp(type, "LT") == 0) res = a.val < b.val;
        else if (std::strcmp(type, "GTE") == 0) res = a.val >= b.val;
        else if (std::strcmp(type, "LTE") == 0) res = a.val <= b.val;
        return createBoolRuleReturn(res);
    }

    return createErrorRuleReturn(UNREC_FUNC_ERROR);
}


