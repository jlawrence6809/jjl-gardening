# Function Registry Refactor

## Overview

Refactor the automation DSL from monolithic function dispatch to a pluggable function registry system. This will improve extensibility, modularity, and maintainability while unifying the syntax for all operations.

## Current State

The DSL currently uses two different mechanisms for evaluation:

1. **Function calls**: Handled by a large if-else chain in `processRuleCore()` (e.g., `["GT", "temperature", 25]`)
2. **Value reading**: Handled by `tryReadValue` callback in `RuleCoreEnv` (e.g., `"temperature"`)

This creates inconsistency and makes adding new functions or sensors require core code changes.

## Target Architecture

### Unified Function System

- Convert all operations to function calls: `"temperature"` → `["getTemperature"]`
- Single evaluation model: everything is a function application
- Pluggable function registration via `RuleCoreEnv`

### Function Registry

- `std::map<std::string, FunctionHandler>` for function lookup
- Functions register themselves through environment callbacks
- Platform-neutral core with platform-specific function registration

### Benefits

- **Extensibility**: Easy to add new functions without core changes
- **Modularity**: Functions implemented independently
- **Testability**: Individual functions can be unit tested
- **Consistency**: Uniform syntax and evaluation model
- **Platform abstraction**: Different boards can register different function sets

## Implementation Plan

### Phase 1: Infrastructure Setup

1. **Create new_core.cpp**: Copy current core.cpp for safe development
2. **Add error codes**: `FUNCTION_NOT_FOUND` to `ErrorCode` enum
3. **Define types**:
   ```cpp
   using FunctionHandler = std::function<UnifiedValue(JsonArrayConst, const RuleCoreEnv&)>;
   using FunctionRegistry = std::map<std::string, FunctionHandler>;
   ```
4. **Update RuleCoreEnv**: Replace `tryReadValue` with function registration callback

### Phase 2: Function Registration System

1. **Registry initialization**: Create and populate function registry
2. **Registration interface**: Environment provides function registration callback
3. **Core integration**: Update rule processing to use registry lookup

### Phase 3: Function Migration

1. **Operator functions**: Migrate `GT`, `LT`, `EQ`, `AND`, `OR`, `NOT`, `IF`, `SET`, `NOP`
2. **Sensor functions**: Create `getTemperature`, `getHumidity`, `getPhotoSensor`, `getLightSwitch`, `getCurrentTime`
3. **Validation helpers**: Common patterns for argument validation

### Phase 4: Integration & Testing

1. **Bridge layer updates**: Register platform-specific functions
2. **Test updates**: Update test suite for new syntax
3. **Rule syntax migration**: Convert existing rules to new format
4. **Cleanup**: Remove old core.cpp, rename new_core.cpp → core.cpp

## Technical Decisions

### Function Namespace

- **Flat namespace**: All functions in single registry (e.g., `GT`, `getTemperature`, `SET`)
- Can be enhanced with categories later if needed

### Argument Passing

- **Current approach**: Functions receive `JsonArrayConst` and parse own arguments
- Enables short-circuit evaluation and conditional argument processing
- Each function handles its own validation

### Registration Strategy

- **Eager registration**: All functions registered at startup
- **Individual registration**: Functions register one at a time
- **Platform-specific**: Bridge layer handles registration through environment

### Error Handling

- **Extend central enum**: Add `FUNCTION_NOT_FOUND` to existing `ErrorCode`
- **Function-specific validation**: Each function uses appropriate existing error codes
- **Future enhancement**: Plan migration to string-based errors later

## File Changes

### New Files

- `src/automation_dsl/new_core.cpp` (temporary development)

### Modified Files

- `src/automation_dsl/unified_value.h` (add `FUNCTION_NOT_FOUND` error)
- `src/automation_dsl/core.h` (update `RuleCoreEnv` structure)
- `src/automation_dsl/bridge.cpp` (replace `tryReadValue` with function registration)
- `tests_native/rule_core_runner.cpp` (update tests for new syntax)

## Example Transformations

### Before (Mixed Syntax)

```json
["IF", ["GT", "temperature", 25], ["SET", "relay_0", 1], ["SET", "relay_0", 0]]
```

### After (Unified Syntax)

```json
[
  "IF",
  ["GT", ["getTemperature"], 25],
  ["SET", "relay_0", 1],
  ["SET", "relay_0", 0]
]
```

## Validation Strategy

### Common Patterns

- Binary numeric operations (GT, LT, etc.)
- Unary numeric operations (NOT)
- Actuator operations (SET)
- Control flow (IF)

### Helper Functions

```cpp
UnifiedValue validateBinaryNumeric(JsonArrayConst args, const RuleCoreEnv& env,
                                   std::function<bool(float,float)> op);
```

## Future Enhancements

### Function Metadata

- Add function descriptions for better error reporting
- Add expected argument counts for uniform validation
- Add function categories/namespaces

### Enhanced Error System

- Migrate from error enums to error strings
- Add error context (which function failed, which argument)
- Function-specific error messages

### Advanced Features

- Function composition and higher-order functions
- User-defined functions
- Function aliases and shortcuts

## Success Criteria

1. **Functional parity**: All existing rules work with new syntax
2. **Test coverage**: All functions individually testable
3. **Platform independence**: Core evaluation logic has no platform dependencies
4. **Extensibility**: New functions can be added without core changes
5. **Performance**: No significant performance regression
6. **Clean code**: Elimination of monolithic if-else dispatch

## Notes

- Use "big bang" migration approach with temporary new_core.cpp for safety
- Maintain existing argument passing strategy for short-circuit evaluation
- Keep error system changes minimal for this phase
- Focus on architecture improvement rather than syntax changes (both old and new syntax can coexist initially)
