/**
 * @file registry_functions.cpp
 * @brief Core function implementations for the automation DSL registry
 *
 * This file implements the standard function set for the automation DSL.
 * Each function follows the FunctionHandler signature and provides specific
 * automation functionality like comparisons, logical operations, and control flow.
 */

#include "registry_functions.h"
#include <cstring>
#include "core.h"

/**
 * @brief Register all core DSL functions into the provided registry
 */
void registerCoreFunctions(FunctionRegistry& registry) {
    // Comparison operators
    registry["GT"] = RegistryFunctions::functionGT;
    registry["LT"] = RegistryFunctions::functionLT;
    registry["EQ"] = RegistryFunctions::functionEQ;
    registry["NE"] = RegistryFunctions::functionNE;
    registry["GTE"] = RegistryFunctions::functionGTE;
    registry["LTE"] = RegistryFunctions::functionLTE;

    // Logical operators
    registry["AND"] = RegistryFunctions::functionAND;
    registry["OR"] = RegistryFunctions::functionOR;
    registry["NOT"] = RegistryFunctions::functionNOT;

    // Control flow
    registry["IF"] = RegistryFunctions::functionIF;

    // Actions
    registry["SET"] = RegistryFunctions::functionSET;
    registry["NOP"] = RegistryFunctions::functionNOP;
}

namespace RegistryFunctions {

/**
 * @brief Helper function for binary numeric operations with comparison
 */
UnifiedValue validateBinaryNumeric(JsonArrayConst args, const RuleCoreEnv& env,
                                   std::function<bool(float, float)> comparison) {
    if (args.size() != 3) {
        return UnifiedValue::createError(COMPARISON_TYPE_ERROR);
    }

    UnifiedValue a = processRuleCore(args[1], env);
    UnifiedValue b = processRuleCore(args[2], env);

    if (a.type == UnifiedValue::ERROR_TYPE) return a;
    if (b.type == UnifiedValue::ERROR_TYPE) return b;

    if ((a.type != UnifiedValue::FLOAT_TYPE && a.type != UnifiedValue::INT_TYPE) ||
        (b.type != UnifiedValue::FLOAT_TYPE && b.type != UnifiedValue::INT_TYPE)) {
        return UnifiedValue::createError(COMPARISON_TYPE_ERROR);
    }

    bool result = comparison(a.asFloat(), b.asFloat());
    return UnifiedValue(result ? 1.0f : 0.0f);
}

/**
 * @brief Helper function for unary numeric operations
 */
UnifiedValue validateUnaryNumeric(JsonArrayConst args, const RuleCoreEnv& env,
                                  std::function<float(float)> operation) {
    if (args.size() != 2) {
        return UnifiedValue::createError(NOT_ERROR);
    }

    UnifiedValue a = processRuleCore(args[1], env);
    if (a.type == UnifiedValue::ERROR_TYPE) return a;

    if (a.type != UnifiedValue::FLOAT_TYPE && a.type != UnifiedValue::INT_TYPE) {
        return UnifiedValue::createError(NOT_ERROR);
    }

    float result = operation(a.asFloat());
    return UnifiedValue(result);
}

// Comparison function implementations

UnifiedValue functionGT(JsonArrayConst args, const RuleCoreEnv& env) {
    return validateBinaryNumeric(args, env, [](float a, float b) { return a > b; });
}

UnifiedValue functionLT(JsonArrayConst args, const RuleCoreEnv& env) {
    return validateBinaryNumeric(args, env, [](float a, float b) { return a < b; });
}

UnifiedValue functionGTE(JsonArrayConst args, const RuleCoreEnv& env) {
    return validateBinaryNumeric(args, env, [](float a, float b) { return a >= b; });
}

UnifiedValue functionLTE(JsonArrayConst args, const RuleCoreEnv& env) {
    return validateBinaryNumeric(args, env, [](float a, float b) { return a <= b; });
}

UnifiedValue functionEQ(JsonArrayConst args, const RuleCoreEnv& env) {
    if (args.size() != 3) {
        return UnifiedValue::createError(COMPARISON_TYPE_ERROR);
    }

    UnifiedValue a = processRuleCore(args[1], env);
    UnifiedValue b = processRuleCore(args[2], env);

    if (a.type == UnifiedValue::ERROR_TYPE) return a;
    if (b.type == UnifiedValue::ERROR_TYPE) return b;

    // EQ supports both numeric and string comparisons
    return UnifiedValue((a == b) ? 1.0f : 0.0f);
}

UnifiedValue functionNE(JsonArrayConst args, const RuleCoreEnv& env) {
    if (args.size() != 3) {
        return UnifiedValue::createError(COMPARISON_TYPE_ERROR);
    }

    UnifiedValue a = processRuleCore(args[1], env);
    UnifiedValue b = processRuleCore(args[2], env);

    if (a.type == UnifiedValue::ERROR_TYPE) return a;
    if (b.type == UnifiedValue::ERROR_TYPE) return b;

    // NE supports both numeric and string comparisons
    return UnifiedValue((a != b) ? 1.0f : 0.0f);
}

// Logical function implementations

UnifiedValue functionAND(JsonArrayConst args, const RuleCoreEnv& env) {
    if (args.size() != 3) {
        return UnifiedValue::createError(AND_OR_ERROR);
    }

    // Evaluate first operand
    UnifiedValue a = processRuleCore(args[1], env);
    if (a.type == UnifiedValue::ERROR_TYPE) return a;

    // Short-circuit evaluation: if first is false, return false immediately
    if (!(a.asFloat() > 0)) {
        return UnifiedValue(0.0f);  // false
    }

    // Evaluate second operand only if first was true
    UnifiedValue b = processRuleCore(args[2], env);
    if (b.type == UnifiedValue::ERROR_TYPE) return b;

    if ((a.type != UnifiedValue::FLOAT_TYPE && a.type != UnifiedValue::INT_TYPE) ||
        (b.type != UnifiedValue::FLOAT_TYPE && b.type != UnifiedValue::INT_TYPE)) {
        return UnifiedValue::createError(AND_OR_ERROR);
    }

    bool result = (a.asFloat() > 0 && b.asFloat() > 0);
    return UnifiedValue(result ? 1.0f : 0.0f);
}

UnifiedValue functionOR(JsonArrayConst args, const RuleCoreEnv& env) {
    if (args.size() != 3) {
        return UnifiedValue::createError(AND_OR_ERROR);
    }

    // Evaluate first operand
    UnifiedValue a = processRuleCore(args[1], env);
    if (a.type == UnifiedValue::ERROR_TYPE) return a;

    // Short-circuit evaluation: if first is true, return true immediately
    if (a.asFloat() > 0) {
        return UnifiedValue(1.0f);  // true
    }

    // Evaluate second operand only if first was false
    UnifiedValue b = processRuleCore(args[2], env);
    if (b.type == UnifiedValue::ERROR_TYPE) return b;

    if ((a.type != UnifiedValue::FLOAT_TYPE && a.type != UnifiedValue::INT_TYPE) ||
        (b.type != UnifiedValue::FLOAT_TYPE && b.type != UnifiedValue::INT_TYPE)) {
        return UnifiedValue::createError(AND_OR_ERROR);
    }

    bool result = (a.asFloat() > 0 || b.asFloat() > 0);
    return UnifiedValue(result ? 1.0f : 0.0f);
}

UnifiedValue functionNOT(JsonArrayConst args, const RuleCoreEnv& env) {
    return validateUnaryNumeric(args, env, [](float a) { return !(a > 0) ? 1.0f : 0.0f; });
}

// Control flow function implementations

UnifiedValue functionIF(JsonArrayConst args, const RuleCoreEnv& env) {
    if (args.size() != 4) {
        return UnifiedValue::createError(IF_CONDITION_ERROR);
    }

    UnifiedValue cond = processRuleCore(args[1], env);
    if (cond.type == UnifiedValue::ERROR_TYPE) return cond;

    if (cond.type != UnifiedValue::FLOAT_TYPE && cond.type != UnifiedValue::INT_TYPE) {
        return UnifiedValue::createError(IF_CONDITION_ERROR);
    }

    // Execute then branch if condition is truthy, else branch otherwise
    return cond.asFloat() > 0 ? processRuleCore(args[2], env) : processRuleCore(args[3], env);
}

// Action function implementations

UnifiedValue functionSET(JsonArrayConst args, const RuleCoreEnv& env) {
    if (args.size() != 3) {
        return UnifiedValue::createError(BOOL_ACTUATOR_ERROR);
    }

    UnifiedValue act = processRuleCore(args[1], env);
    UnifiedValue val = processRuleCore(args[2], env);

    if (act.type == UnifiedValue::ERROR_TYPE) return act;
    if (val.type == UnifiedValue::ERROR_TYPE) return val;

    if (act.type != UnifiedValue::ACTUATOR_TYPE ||
        (val.type != UnifiedValue::FLOAT_TYPE && val.type != UnifiedValue::INT_TYPE)) {
        return UnifiedValue::createError(BOOL_ACTUATOR_ERROR);
    }

    auto setter = act.getActuatorSetter();
    if (setter) setter(val.asFloat());

    return UnifiedValue::createVoid();
}

UnifiedValue functionNOP(JsonArrayConst args, const RuleCoreEnv& env) {
    // NOP takes no arguments and does nothing
    return UnifiedValue::createVoid();
}

}  // namespace RegistryFunctions
