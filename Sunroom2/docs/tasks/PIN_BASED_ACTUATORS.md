# Pin-Based Actuators with LOOKUP_PIN TODO

## Goal

Refactor actuator system from index-based naming (`"relay_0"`) to pin-based addressing with optional friendly name lookup.

## Current Problems

- **Hardcoded naming**: `"relay_" + index` assumes relays are only actuators
- **Fragile references**: Rules break when relays are reordered or deleted
- **Index coupling**: Rule references tied to internal array positions
- **Limited extensibility**: Hard to add non-relay actuators

## Proposed Solution

### Pin-Based Direct Addressing

```json
// Direct pin reference (always stable)
["SET", "pin_23", 1]
["SET", "pin_18", 0]
```

### Friendly Name Lookup Function

```json
// User-friendly names via lookup
["SET", ["LOOKUP_PIN", "sunroom_lights"], 1]
["SET", ["LOOKUP_PIN", "exhaust_fan"], 0]

// Can be used in any expression context
["IF", ["GT", "temperature", 25],
  ["SET", ["LOOKUP_PIN", "cooling_fan"], 1],
  ["SET", ["LOOKUP_PIN", "cooling_fan"], 0]
]
```

## Implementation Design

### LOOKUP_PIN Function

```cpp
// In rule_core.cpp, add new function type
else if (std::strcmp(type, "LOOKUP_PIN") == 0)
{
    // Get friendly name from array[1]
    const char* friendlyName = array[1].as<const char*>();

    // Look up pin number from name mapping
    int pin = env.lookupPin ? env.lookupPin(friendlyName) : -1;

    return pin < 0 ? createErrorRuleReturn(LOOKUP_ERROR) : createIntRuleReturn(pin);
}
```

### Updated Environment

```cpp
struct RuleCoreEnv {
    // Existing members...

    // New lookup function
    std::function<int(const std::string &friendlyName)> lookupPin;
};
```

### Name-to-Pin Mapping Storage

Store in preferences as JSON:

```json
{
  "sunroom_lights": 23,
  "exhaust_fan": 18,
  "heater": 19,
  "water_pump": 21
}
```

### Updated Actuator Resolution

```cpp
// In rule processing, handle both patterns:
if (str.rfind("pin_", 0) == 0) {
    // Direct pin: "pin_23" → pin 23
    int pin = atoi(str.c_str() + 4);
    setter = [pin](float v) { setPinValue(pin, v); };
    return true;
}
```

## Migration Strategy

### Phase 1: Support Both Systems

- Keep existing `"relay_0"` support for backward compatibility
- Add `"pin_XX"` support for direct addressing
- Add `LOOKUP_PIN` function

### Phase 2: UI Updates

- Allow users to assign friendly names to pins in relay management UI
- Show both pin number and friendly name in UI
- Generate rules using pin-based format for new relays

### Phase 3: Migration Tools

- Provide conversion utility for existing rules
- Gradually migrate system rules to pin-based format
- Eventually deprecate index-based references

## Benefits

### Stability

- **Pin references never break**: Physical pins don't change
- **Deletion safe**: Removing a relay doesn't affect other rule references
- **Reorder safe**: Pin 23 is always pin 23 regardless of relay list order

### Flexibility

- **Multiple actuator types**: Not limited to relays (`"pin_XX"` works for any GPIO)
- **User-friendly**: `LOOKUP_PIN` allows meaningful names in rules
- **Extensible**: Easy to add other lookup types (device IDs, etc.)

### Maintainability

- **Clear semantics**: Pin references are unambiguous
- **Easier debugging**: Pin numbers directly correlate to hardware
- **Future-proof**: Works with any pin-based actuator system

## Example Use Cases

### Mixed Addressing

```json
// Critical systems use stable pin references
["IF", ["GT", "temperature", 35], ["SET", "pin_23", 1]]

// User-friendly rules use lookup
["IF", ["EQ", "lightSwitch", 1],
  ["SET", ["LOOKUP_PIN", "sunroom_lights"], 1]
]
```

### Error Handling

```json
// LOOKUP_PIN returns error if name not found
[
  "IF",
  ["EQ", "lightSwitch", 1],
  ["SET", ["LOOKUP_PIN", "nonexistent_light"], 1]
]
// → Error type returned, rule fails gracefully
```

### Dynamic Rules

```json
// Users can change friendly names without breaking rules
["SET", ["LOOKUP_PIN", "main_light"], 1]
// Works regardless of which pin "main_light" maps to
```

## Implementation Notes

- **Thread safety**: Ensure atomic updates to name mapping
- **Persistence**: Store name mappings in NVS preferences
- **Validation**: Verify pin numbers are valid GPIO pins
- **Error handling**: Graceful failure for invalid pins/names
- **Testing**: Add comprehensive tests for both addressing modes
