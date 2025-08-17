# Relay System Simplification Design Document

## Overview

The current relay system uses a complex two-digit encoding (force/auto) that was originally designed to work around limitations in the rule language. With the planned `LAST` function enabling event-based logic, we can simplify this system significantly.

## Current System Problems

### Complex Two-Digit Encoding

The current `RelayValue` uses a two-digit system:

- **Tens digit** = "auto" value (set by rules): 0=off, 1=on, 2=don't care
- **Ones digit** = "force" value (set by UI/switches): 0=off, 1=on, 2=don't care

This creates 9 possible states, most of which are confusing:

```cpp
// From definitions.h
typedef enum {
    FORCE_OFF_AUTO_OFF = 00, FORCE_OFF_AUTO_ON = 10, FORCE_OFF_AUTO_X = 20,
    FORCE_ON_AUTO_OFF = 01,  FORCE_ON_AUTO_ON = 11,  FORCE_ON_AUTO_X = 21,
    FORCE_X_AUTO_OFF = 02,   FORCE_X_AUTO_ON = 12,   FORCE_X_AUTO_X = 22
} RelayValue;
```

### Over-Engineering

The system was designed to handle state transitions that the `LAST` function will solve more elegantly:

- **Current approach**: Complex state encoding + "don't care" initialization
- **Better approach**: Simple boolean state + event-based rules

### UI Complexity

The tri-state buttons map awkwardly to the two-digit system, leading to user confusion about what each state means.

## Target System Design

### Simple Relay Model

Replace `RelayValue` with a simpler structure:

```cpp
struct RelayState {
    bool isOn;           // Current physical state
    bool manualOverride; // True if manually controlled, false if rule-controlled
};
```

Or even simpler - just a boolean for rule-controlled relays:

```cpp
bool RELAY_STATES[MAX_RELAYS];  // Simple on/off state
```

### Rule-Based Manual Control

Instead of encoding manual overrides in the relay state, handle them through rules:

```cpp
// Light switch rule with LAST function
[
  "OR",
  // Switch was flipped ON
  ["AND", ["EQ", "lightSwitch", 1], ["EQ", ["LAST", "lightSwitch"], 0]],
  // Keep current state if switch unchanged and was previously on
  ["AND", ["EQ", "lightSwitch", 1], ["EQ", ["LAST", "lightSwitch"], 1]]
]
```

### UI Simplification

Replace tri-state buttons with clearer options:

**Option A: Auto/Manual Toggle**

- Toggle between "Auto" (rule-controlled) and "Manual" modes
- When in Manual mode, show simple On/Off buttons

**Option B: Override Buttons**

- "Auto" mode by default (rules control the relay)
- "Force On" and "Force Off" buttons for temporary overrides
- Clear indication of override state and easy way to return to auto

**Option C: Rule-Only**

- Remove manual controls entirely
- All control happens through rules (including switch inputs)
- Simplest but least flexible for debugging

## Migration Strategy

### Phase 1: Implement LAST Function

**Prerequisite**: Must have event-based logic before removing force/auto system

- Add `LAST` function to rule engine
- Add sensor state memory to track previous values
- Update rule language documentation

### Phase 2: Migrate Existing Rules

**Convert current patterns to event-based patterns**:

Before (implicit relay control with force/auto):

```json
["GT", "temperature", 25]
```

After (explicit event-based control):

```json
[
  "IF",
  [
    "OR",
    ["AND", ["GT", "temperature", 25], ["LTE", ["LAST", "temperature"], 25]],
    ["AND", ["GT", "temperature", 25], ["GT", ["LAST", "temperature"], 25]]
  ],
  1,
  0
]
```

Or maintain simple implicit control for non-event cases:

```json
["GT", "temperature", 25] // Still works, just with simpler underlying system
```

### Phase 3: Simplify Relay State

**Replace two-digit system**:

1. Update `RelayValue` enum to simple boolean or tri-state
2. Simplify `setRelay()` function
3. Update `relayRefresh()` logic
4. Remove complex state encoding

### Phase 4: Update UI

**Simplify frontend controls**:

1. Design new button layout (Auto/Manual or Override approach)
2. Update API endpoints for new state model
3. Test all interaction patterns

### Phase 5: Clean Up

**Remove legacy code**:

1. Delete unused `RelayValue` enum values
2. Clean up complex state logic in `peripheral_controls.cpp`
3. Update documentation and tests

## Design Decisions Needed

### 1. Manual Override Philosophy

**Question**: How should manual overrides work?

**Option A: Temporary Override**

- Manual control lasts until next rule evaluation
- Good for debugging, rules always "win" eventually
- Simple to implement

**Option B: Persistent Override**

- Manual control persists until user explicitly returns to auto
- Good for user control, but can lead to "forgotten" overrides
- Need timeout or clear indication

**Option C: Rule Integration**

- Manual inputs (switches, buttons) are just sensor inputs to rules
- All logic handled consistently through rules
- Most flexible but requires more rule complexity

### 2. Backward Compatibility

**Question**: Do we need to support both systems during migration?

**Recommendation**: Implement migration tools but don't maintain dual systems long-term. The complexity isn't worth it for a personal project.

### 3. Default Behavior

**Question**: What happens when no rules apply to a relay?

**Options**:

- Default to OFF (current behavior)
- Default to last known state
- Default to user-configurable value
- Require explicit rule for every relay

## Benefits of Simplification

### Developer Benefits

- **Easier debugging**: Simple boolean state vs. complex encoding
- **Clearer code**: Obvious what each state means
- **Better testability**: Fewer edge cases and state combinations
- **Maintainability**: Less complex state logic to maintain

### User Benefits

- **Clearer UI**: Obvious what buttons do
- **Predictable behavior**: No mysterious state combinations
- **Better error recovery**: Simpler to understand and fix issues

### System Benefits

- **Performance**: Simpler state checks and updates
- **Memory**: Less complex state storage
- **Event-based logic**: Natural fit with `LAST` function capabilities

## Implementation Notes

### Testing Strategy

- Implement comprehensive tests for new relay model
- Test migration from old to new system
- Verify all existing use cases work with new system

### Migration Tools

Consider building tools to:

- Convert existing rules to new format
- Validate that new system produces same results as old system
- Help users understand the new model

### Documentation Updates

- Update rule language documentation
- Create user guide for new UI
- Document migration process for future reference

## Timeline

This is a significant change that touches many parts of the system. Recommended approach:

1. **Week 1-2**: Implement and test `LAST` function
2. **Week 3**: Design and prototype new relay model
3. **Week 4**: Implement migration tools and test with existing rules
4. **Week 5**: Update UI and API endpoints
5. **Week 6**: Testing, documentation, and cleanup

Total estimated effort: ~6 weeks for complete migration.

## Future Considerations

### Extensibility

The simplified system should make it easier to add:

- Different actuator types (PWM, servo, etc.)
- More complex rule patterns
- Better debugging and monitoring tools

### Scaling

If the system grows to support many relays, consider:

- Bulk state operations
- Rule optimization for performance
- State persistence and recovery strategies
