#pragma once

#include "new_core.h"

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
UnifiedValue functionGT(JsonArrayConst args, const NewRuleCoreEnv& env);
UnifiedValue functionLT(JsonArrayConst args, const NewRuleCoreEnv& env);
UnifiedValue functionEQ(JsonArrayConst args, const NewRuleCoreEnv& env);
UnifiedValue functionNE(JsonArrayConst args, const NewRuleCoreEnv& env);
UnifiedValue functionGTE(JsonArrayConst args, const NewRuleCoreEnv& env);
UnifiedValue functionLTE(JsonArrayConst args, const NewRuleCoreEnv& env);

// Logical functions
UnifiedValue functionAND(JsonArrayConst args, const NewRuleCoreEnv& env);
UnifiedValue functionOR(JsonArrayConst args, const NewRuleCoreEnv& env);
UnifiedValue functionNOT(JsonArrayConst args, const NewRuleCoreEnv& env);

// Control flow functions
UnifiedValue functionIF(JsonArrayConst args, const NewRuleCoreEnv& env);

// Action functions
UnifiedValue functionSET(JsonArrayConst args, const NewRuleCoreEnv& env);
UnifiedValue functionNOP(JsonArrayConst args, const NewRuleCoreEnv& env);

// Helper functions for common validation patterns
UnifiedValue validateBinaryNumeric(JsonArrayConst args, const NewRuleCoreEnv& env,
                                   std::function<bool(float, float)> comparison);
UnifiedValue validateUnaryNumeric(JsonArrayConst args, const NewRuleCoreEnv& env,
                                  std::function<float(float)> operation);
}  // namespace RegistryFunctions
