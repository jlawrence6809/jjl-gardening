#pragma once

#include "core.h"

/**
 * @file registry_functions.h
 * @brief Core function implementations for the automation DSL registry
 *
 * This file contains the standard function implementations that are registered
 * with the function registry. These include:
 * - Comparison operators (GT, LT, EQ, etc.)
 * - Logical operators (AND, OR, NOT)
 * - Control flow (IF)
 * - Actions (SET, NOP)
 * - Helper functions for common validation patterns
 */

/**
 * @brief Register all core DSL functions into the provided registry
 * @param registry The function registry to populate with core functions
 *
 * This function registers all the standard automation DSL functions:
 * - Comparison: GT, LT, EQ, NE, GTE, LTE
 * - Logical: AND, OR, NOT
 * - Control: IF
 * - Actions: SET, NOP
 */
void registerCoreFunctions(FunctionRegistry& registry);

// Individual function implementations
namespace RegistryFunctions {

// Comparison functions
UnifiedValue functionGT(JsonArrayConst args, const RuleCoreEnv& env);
UnifiedValue functionLT(JsonArrayConst args, const RuleCoreEnv& env);
UnifiedValue functionEQ(JsonArrayConst args, const RuleCoreEnv& env);
UnifiedValue functionNE(JsonArrayConst args, const RuleCoreEnv& env);
UnifiedValue functionGTE(JsonArrayConst args, const RuleCoreEnv& env);
UnifiedValue functionLTE(JsonArrayConst args, const RuleCoreEnv& env);

// Logical functions
UnifiedValue functionAND(JsonArrayConst args, const RuleCoreEnv& env);
UnifiedValue functionOR(JsonArrayConst args, const RuleCoreEnv& env);
UnifiedValue functionNOT(JsonArrayConst args, const RuleCoreEnv& env);

// Control flow functions
UnifiedValue functionIF(JsonArrayConst args, const RuleCoreEnv& env);

// Action functions
UnifiedValue functionSET(JsonArrayConst args, const RuleCoreEnv& env);
UnifiedValue functionNOP(JsonArrayConst args, const RuleCoreEnv& env);

// Helper functions for common validation patterns
UnifiedValue validateBinaryNumeric(JsonArrayConst args, const RuleCoreEnv& env,
                                   std::function<bool(float, float)> comparison);
UnifiedValue validateUnaryNumeric(JsonArrayConst args, const RuleCoreEnv& env,
                                  std::function<float(float)> operation);
}  // namespace RegistryFunctions
