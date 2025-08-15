# Sunroom2 Rule Language Reference

A LISP-like JSON-based automation rule language for IoT devices.

## Overview

The Sunroom2 rule language allows you to create automation rules using a functional programming style with JSON syntax. Rules are expressed as nested arrays where the first element is always the function name, followed by its arguments.

### Implicit Relay Control

**Important**: When a rule is associated with a specific relay (e.g., `relay_0`, `relay_1`), the rule engine automatically sets that relay to the final result value if the outermost expression evaluates to a numeric value:

- If the rule returns a **float/number**: The relay is set to that value (typically 0 or 1)
- If the rule returns **void** (from `SET` or `NOP`): No automatic relay setting occurs
- If the rule returns an **error**: The relay is not modified

This means you can write simple rules like:

```json
["GT", "temperature", 25]
```

And the system will automatically set the associated relay to `1` when temperature > 25°C, or `0` otherwise.

## Basic Syntax

All expressions follow this pattern:

```json
["FUNCTION_NAME", arg1, arg2, ...]
```

## Data Types

### Literals

- **Numbers**: `25`, `20.5`, `-10`
- **Booleans**: `true`, `false`
  - **Note**: Booleans are syntax sugar for floats (`true` = `1.0`, `false` = `0.0`)
- **Time Literals**: `"@HH:MM:SS"` (e.g., `"@14:30:00"`)

### Sensors

- `"temperature"` - Temperature sensor reading (float)
- `"humidity"` - Humidity sensor reading (float)
- `"photoSensor"` - Light sensor reading (float)
- `"lightSwitch"` - Physical switch state (0/1)
- `"currentTime"` - Current time in seconds since midnight

### Actuators

- `"relay_0"`, `"relay_1"`, ... - Controllable relays

## Operators

### Comparison Operators

All comparison operators return `1.0` for true, `0.0` for false (internally stored as floats).

- **`EQ`** - Equal to

  ```json
  ["EQ", "temperature", 25]
  ["EQ", true, true]
  ```

- **`NE`** - Not equal to

  ```json
  ["NE", "humidity", 50]
  ```

- **`GT`** - Greater than

  ```json
  ["GT", "temperature", 20]
  ["GT", "currentTime", "@12:00:00"]
  ```

- **`LT`** - Less than

  ```json
  ["LT", "photoSensor", 1000]
  ```

- **`GTE`** - Greater than or equal

  ```json
  ["GTE", "temperature", 20]
  ```

- **`LTE`** - Less than or equal
  ```json
  ["LTE", "humidity", 80]
  ```

### Logical Operators

- **`AND`** - Logical AND (supports short-circuiting)

  ```json
  ["AND", ["GT", "temperature", 20], ["LT", "humidity", 80]]
  ```

- **`OR`** - Logical OR (supports short-circuiting)

  ```json
  ["OR", ["GT", "temperature", 30], ["LT", "humidity", 60]]
  ```

- **`NOT`** - Logical NOT
  ```json
  ["NOT", ["EQ", "lightSwitch", 1]]
  ```

### Control Flow

- **`IF`** - Conditional execution

  ```json
  ["IF", condition, then_expression, else_expression]
  ```

  Example:

  ```json
  [
    "IF",
    ["GT", "temperature", 25],
    ["SET", "relay_0", 1],
    ["SET", "relay_0", 0]
  ]
  ```

### Actions

- **`SET`** - Set actuator value

  ```json
  ["SET", "relay_0", 1]
  ["SET", "relay_1", 0]
  ```

- **`NOP`** - No operation (does nothing)
  ```json
  ["NOP"]
  ```

## Example Rules

### Temperature Control

**Simple approach** (using implicit relay control):

```json
["GT", "temperature", 25]
```

This automatically sets the associated relay to `1` when temperature > 25°C, `0` otherwise.

**Explicit approach** (using IF-SET):

```json
["IF", ["GT", "temperature", 25], ["SET", "relay_0", 1], ["SET", "relay_0", 0]]
```

This gives you more control but requires explicit `SET` statements.

### Time-Based Control

**Simple approach**:

```json
["GT", "currentTime", "@18:00:00"]
```

Automatically turns on relay after 6:00 PM.

**Explicit approach**:

```json
[
  "IF",
  ["GT", "currentTime", "@18:00:00"],
  ["SET", "relay_0", 1],
  ["SET", "relay_0", 0]
]
```

### Complex Conditions

**Simple approach**:

```json
["AND", ["GT", "temperature", 28], ["LT", "humidity", 40]]
```

Automatically turns on relay when temperature > 28°C AND humidity < 40%.

**Explicit approach**:

```json
[
  "IF",
  ["AND", ["GT", "temperature", 28], ["LT", "humidity", 40]],
  ["SET", "relay_0", 1],
  ["SET", "relay_0", 0]
]
```

### Range-Based Control

Turn on heating when temperature is between 18-22°C:

```json
[
  "IF",
  ["AND", ["GTE", "temperature", 18], ["LTE", "temperature", 22]],
  ["SET", "relay_0", 1],
  ["SET", "relay_0", 0]
]
```

### Light-Dependent Control

Turn on lights when it's dark OR when the switch is on:

```json
[
  "IF",
  ["OR", ["LT", "photoSensor", 500], ["EQ", "lightSwitch", 1]],
  ["SET", "relay_0", 1],
  ["SET", "relay_0", 0]
]
```

## Error Handling

The rule engine will return errors for:

- **Unknown sensors**: Referencing non-existent sensor names
- **Unknown actuators**: Trying to set non-existent actuators
- **Type mismatches**: Using incompatible types in operations
- **Invalid time literals**: Malformed time strings
- **Syntax errors**: Invalid JSON or function names

## Best Practices

1. **Choose the right approach**: Use simple expressions (e.g., `["GT", "temperature", 25]`) for straightforward on/off control. Use explicit `IF`-`SET` when you need complex logic or multiple relay control.

2. **Use descriptive sensor names**: Stick to the predefined sensor names

3. **Validate time format**: Always use `@HH:MM:SS` for time literals

4. **Handle edge cases**: Consider boundary conditions in your comparisons

5. **Keep rules simple**: Break complex logic into smaller, testable rules

6. **Test thoroughly**: Use the native test runner to validate rule behavior

7. **Understand return types**: Remember that simple expressions return numeric values that automatically control the relay, while `SET` operations return void and don't trigger automatic relay control.

## Implementation Notes

- **Unified numeric system**: All values are internally stored as floats, including booleans (`true`=`1.0`, `false`=`0.0`) and integers
- All numeric comparisons are performed using floating-point arithmetic
- Time values are internally converted to seconds since midnight
- Boolean expressions return `1.0` for true and `0.0` for false
- Short-circuit evaluation is used for `AND` and `OR` operations
- The rule engine is platform-neutral and can run both on ESP32 and native systems
