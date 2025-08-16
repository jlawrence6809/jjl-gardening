# LAST Function Design Document

## Overview

The `LAST` function extends the Sunroom2 rule language to support **edge detection** and **state transition logic** by providing access to sensor values from the previous rule evaluation cycle.

## Problem Statement

The current rule language only supports continuous state evaluation:

- `["GT", "temperature", 25]` means "while temperature > 25°C, keep relay on"
- `["EQ", "lightSwitch", 1]` means "while switch is on, keep relay on"

Missing functionality:

- **Event-based triggers**: "When switch flips from off→on, turn relay on"
- **Time transitions**: "At the moment we reach 5pm, turn relay on"
- **Threshold crossings**: "When temperature crosses 25°C going up, turn relay on"

## Solution: LAST Function

### Syntax

```json
["LAST", "sensor_name"]
```

Returns the value of the specified sensor from the previous rule evaluation cycle.

### Supported Sensors

- `"temperature"` - Previous temperature reading
- `"humidity"` - Previous humidity reading
- `"photoSensor"` - Previous light sensor reading
- `"lightSwitch"` - Previous switch state
- `"currentTime"` - Previous time value

## Edge Detection Patterns

### Light Switch Edge Detection

```json
// Switch flipped ON (off→on)
["AND", ["EQ", "lightSwitch", 1], ["EQ", ["LAST", "lightSwitch"], 0]]

// Switch flipped OFF (on→off)
["AND", ["EQ", "lightSwitch", 0], ["EQ", ["LAST", "lightSwitch"], 1]]

// Switch changed (any direction)
["NE", "lightSwitch", ["LAST", "lightSwitch"]]
```

### Time-Based Events

```json
// Just reached 5:00 PM
["AND",
  ["GTE", "currentTime", "@17:00:00"],
  ["LT", ["LAST", "currentTime"], "@17:00:00"]
]

// Just reached 10:00 PM
["AND",
  ["GTE", "currentTime", "@22:00:00"],
  ["LT", ["LAST", "currentTime"], "@22:00:00"]
]
```

### Temperature Threshold Crossings

```json
// Temperature crossed 25°C going UP
["AND", ["GT", "temperature", 25], ["LTE", ["LAST", "temperature"], 25]]

// Temperature crossed 25°C going DOWN
["AND", ["LT", "temperature", 25], ["GTE", ["LAST", "temperature"], 25]]
```

### Complex Event Logic

```json
// Turn on when switch flips OR when we reach 5pm
[
  "OR",
  ["AND", ["EQ", "lightSwitch", 1], ["EQ", ["LAST", "lightSwitch"], 0]],
  [
    "AND",
    ["GTE", "currentTime", "@17:00:00"],
    ["LT", ["LAST", "currentTime"], "@17:00:00"]
  ]
]
```

## Implementation Design

### Memory Requirements

Store one previous value per sensor:

```cpp
struct LastSensorValues {
    float lastTemperature;
    float lastHumidity;
    float lastPhotoSensor;
    float lastLightSwitch;
    int lastCurrentTime;
};
```

### Update Timing

1. **During Rule Evaluation**: `LAST` functions return stored values from previous cycle
2. **After All Rules Complete**: Update stored values with current sensor readings
3. **Bootstrap**: Use default values for the very first evaluation

### Default Values (Bootstrap)

For the first rule evaluation when no previous values exist:

| Sensor        | Default Value | Rationale                     |
| ------------- | ------------- | ----------------------------- |
| `temperature` | `20.0`        | Room temperature baseline     |
| `humidity`    | `50.0`        | Moderate humidity baseline    |
| `photoSensor` | `500.0`       | Mid-range light level         |
| `lightSwitch` | `0.0`         | Switch starts off             |
| `currentTime` | Current time  | No meaningful "previous" time |

### Type Consistency

- `LAST` returns the same type as the corresponding sensor
- Works seamlessly with all existing comparison operators
- Maintains float-based numeric system

## Future Extensions

### BECAME Syntactic Sugar

Once `LAST` is implemented, add convenience functions:

```json
// Syntactic sugar for common patterns
["BECAME", "lightSwitch", 1]  // equivalent to switch flip detection
["BECAME_GT", "temperature", 25]  // equivalent to threshold crossing up
["BECAME_TIME", "@17:00:00"]  // equivalent to time transition
```

### Extended Sensor Support

When new sensors are added, extend `LAST` support with appropriate defaults:

```json
["LAST", "motionSensor"]    // default: 0 (no motion)
["LAST", "doorSensor"]      // default: 0 (closed)
```

## Benefits

1. **Composable**: Works with existing operators and functions
2. **Intuitive**: Easy to understand and reason about
3. **Memory Efficient**: Minimal state storage required
4. **Backward Compatible**: Existing rules continue to work unchanged
5. **Powerful**: Enables complex event-based automation logic

## Example Use Cases

### Smart Lighting

```json
// Turn on lights when switch flips OR it gets dark
[
  "OR",
  ["AND", ["EQ", "lightSwitch", 1], ["EQ", ["LAST", "lightSwitch"], 0]],
  ["AND", ["LT", "photoSensor", 300], ["GTE", ["LAST", "photoSensor"], 300]]
]
```

### Temperature Control with Hysteresis

```json
// Turn on heating when temp drops below 20°C, off when above 22°C
[
  "OR",
  ["AND", ["LT", "temperature", 20], ["GTE", ["LAST", "temperature"], 20]],
  [
    "AND",
    [
      "NOT",
      ["OR", ["GT", "temperature", 22], ["LTE", ["LAST", "temperature"], 22]]
    ],
    "currentRelayState"
  ]
]
```

### Daily Schedule Events

```json
// Morning routine: turn on at 7am, turn off at 9am
[
  "OR",
  [
    "AND",
    ["GTE", "currentTime", "@07:00:00"],
    ["LT", ["LAST", "currentTime"], "@07:00:00"]
  ],
  [
    "AND",
    ["GTE", "currentTime", "@09:00:00"],
    ["LT", ["LAST", "currentTime"], "@09:00:00"]
  ]
]
```

## Implementation Notes

- **Thread Safety**: Ensure atomic updates of last values
- **Persistence**: Consider whether last values should survive device reboot
- **Testing**: Add comprehensive edge detection tests to native test suite
- **Documentation**: Update rule language reference with `LAST` examples
