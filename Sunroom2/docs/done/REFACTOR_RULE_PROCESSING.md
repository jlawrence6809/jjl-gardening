# Rule Processing Refactor TODO

## Goal

Move generic rule processing logic from `rule_helpers.cpp` to `rule_core.cpp` for better testability and separation of concerns.

## Current State

`processAutomationDsl()` in `rule_helpers.cpp` contains both:

- **Application-specific code**: Arduino sensor getters, relay setters, environment setup
- **Generic rule processing**: JSON parsing, rule iteration, result handling

## Proposed Changes

### New Function in `rule_core.cpp`

```cpp
void processRuleSet(
    const String rules[],
    int ruleCount,
    const RuleCoreEnv &env,
    std::function<void(int index, float value)> setActuator
);
```

### Logic to Move

From `processAutomationDsl()` to the new `processRuleSet()`:

1. **Rule iteration loop** (lines 149-212)
2. **JSON deserialization** (lines 158-166)
3. **Result type handling**:
   - `FLOAT_TYPE` → call `setActuator(index, value)`
   - `VOID_TYPE` → continue
   - `ERROR_TYPE` → log error
4. **Error logging** for unexpected results

### Logic to Keep in `rule_helpers.cpp`

Application-specific Arduino environment setup:

1. **Sensor getters**: `getTemperature()`, `getHumidity()`, etc.
2. **Actuator setter**: `setRelay()` function
3. **Environment bridging**: Lambda functions in `RuleCoreEnv`
4. **Call to new function**: `processRuleSet(RELAY_RULES, RUNTIME_RELAY_COUNT, env, setRelay)`

## Benefits

- **Better testability**: Generic rule processing can be tested natively
- **Cleaner separation**: Platform-specific vs platform-neutral code
- **Easier to extend**: Future rule processing features in reusable core
- **Simplified debugging**: Rule logic isolated from Arduino environment

## Implementation Notes

- Maintain backward compatibility
- Ensure error handling is preserved
- Keep existing serial logging behavior
- Test both embedded and native environments
