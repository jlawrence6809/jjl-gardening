# Variant Value System TODO

## Goal

Replace the current "everything is float" sensor value system with a more flexible variant type system that can handle floats, integers, and strings.

## Current State

- All sensor values are converted to `float`
- `currentTime` returns `int` but gets converted to `float`
- Type system is simple but limited for future extensions
- Cannot handle string sensor values (e.g., device status, modes)

## Proposed Variant Type System

### Core Variant Structure

```cpp
struct ValueTaggedUnion {
    enum Type { FLOAT, INT, STRING } type;
    union {
        float f;
        int i;
        const char* s;  // or std::string for safer memory management
    } value;

    // Constructor overloads for easy creation
    ValueTaggedUnion(float val) : type(FLOAT) { value.f = val; }
    ValueTaggedUnion(int val) : type(INT) { value.i = val; }
    ValueTaggedUnion(const char* val) : type(STRING) { value.s = val; }

    // Conversion methods with type checking
    float asFloat() const;
    int asInt() const;
    const char* asString() const;

    // Automatic conversion for compatibility
    operator float() const { return asFloat(); }
};
```

### Updated Sensor Interface

```cpp
// Update RuleCoreEnv to use variant values
struct RuleCoreEnv {
    std::function<bool(const std::string &name, ValueTaggedUnion &outVal)> tryReadSensor;
    // ... other members unchanged
};
```

### Automatic Type Conversion

For backward compatibility and rule simplicity:

- `INT` ↔ `FLOAT` conversion (lossless for typical sensor ranges)
- `STRING` → `FLOAT` parsing where possible ("25.3" → 25.3)
- Comparison operators work across compatible types

## Future Sensor Examples

### String-Based Sensors

```json
// Device status sensors
["EQ", "wifi_status", "connected"]
["EQ", "device_mode", "heating"]

// Enum-like states
["EQ", "system_state", "idle"]
["NE", "error_status", "none"]
```

### Mixed Type Rules

```json
// Temperature (float) with mode (string)
["AND", ["GT", "temperature", 25.0], ["EQ", "hvac_mode", "cooling"]]
```

## Implementation Strategy

### Phase 1: Internal Refactor

1. Implement `ValueTaggedUnion` variant type
2. Update `RuleCoreEnv` to use variants internally
3. Maintain float-based external API for compatibility
4. Add automatic conversion logic

### Phase 2: Extended Sensor Support

1. Add string sensor support to rule processing
2. Update comparison operators for cross-type operations
3. Add string-specific operators (`CONTAINS`, `STARTS_WITH`, etc.)

### Phase 3: Rule Language Extensions

1. Update rule language documentation
2. Add string literal support in JSON rules
3. Extend native test suite for mixed types

## Memory/Performance Considerations

### Memory Overhead

- **Type enum**: 1-4 bytes (depending on alignment)
- **Union storage**: Size of largest member (likely 8 bytes for string pointer)
- **Total per value**: ~12 bytes vs current 4 bytes (3x increase)

### Performance Impact

- **Type checking**: Branch per access (~1-2 CPU cycles)
- **Conversion overhead**: Minimal for numeric types
- **String operations**: Only when actually using strings

### ESP32 Resources

- **RAM**: Typically 320KB+ available, sensor values are small portion
- **CPU**: 240MHz dual-core, type checking overhead negligible
- **Conclusion**: Performance impact acceptable for flexibility gained

## Backward Compatibility

### Existing Rules

All current rules continue to work unchanged:

```json
["GT", "temperature", 25] // Works exactly as before
```

### Sensor APIs

Current sensor getters (`getTemperature()` → `float`) remain unchanged, just wrapped in variant constructors internally.

## Benefits

1. **Future-proof**: Support for any sensor value type
2. **Type safety**: Compile-time and runtime type checking
3. **Flexibility**: Rules can handle diverse sensor ecosystems
4. **Performance**: Automatic conversions minimize rule complexity
5. **Extensible**: Easy to add new types (dates, enums, etc.)

## Alternative: Gradual Migration

If variant system seems too complex initially:

1. **Keep "everything is float" for now**
2. **Add string sensor support as special case**
3. **Migrate to variants when string usage increases**

This allows deferring the complexity while tracking the requirement.
