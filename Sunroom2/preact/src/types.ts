// Types to implement "if this then that" rules for various sensors/actuators
// Eg: temperature (c/f), humidity (rh%), light (adc value), timestamp, etc.

export type NominalType<T, Name extends string> = T & { __nominal: Name };
export type NominalTimestamp = NominalType<number, 'Timestamp'>;

export type NominalSensor = NominalType<string, 'Sensor'>; // eg: 'temperature', 'humidity', 'light', etc.
export type NominalActuator = NominalType<string, 'Actuator'>; // eg: 'led', 'fan', 'heater', etc.

// ~~~~~~~~~~~~~ Primitives ~~~~~~~~~~~~~
export type BooleanValue = {
  type: 'bool';
  value: boolean;
};

export type IntValue = {
  type: 'int';
  value: number;
};

export type FloatValue = {
  type: 'float';
  value: number;
};

export type TimestampValue = {
  type: 'timestamp';
  value: NominalTimestamp;
};

export type Value = BooleanValue | IntValue | FloatValue | TimestampValue;
export type PrimitiveType = Value['type'];

// ~~~~~~~~~~~~~ Conditions ~~~~~~~~~~~~~

export type BooleanComparison = 'EQ' | 'NE';
export type NumberComparison = 'EQ' | 'NE' | 'GT' | 'LT' | 'GTE' | 'LTE';

export type BooleanCondition = BooleanValue & {
  input: NominalSensor | NominalActuator;
  comparison: BooleanComparison;
};

export type IntCondition = IntValue & {
  input: NominalSensor | NominalActuator;
  comparison: NumberComparison;
};

export type FloatCondition = FloatValue & {
  input: NominalSensor | NominalActuator;
  comparison: NumberComparison;
};

export type TimestampCondition = TimestampValue & {
  input: NominalSensor | NominalActuator;
  comparison: NumberComparison;
};

export type LogicalOperator = 'AND' | 'OR';
export type CompoundCondition = {
  type: 'compound';
  operator: LogicalOperator;
  conditions: Condition[];
};

// Extending the Condition type to include CompoundCondition
export type Condition =
  | BooleanCondition
  | IntCondition
  | FloatCondition
  | TimestampCondition
  | CompoundCondition;

// ~~~~~~~~~~~~~ Rules ~~~~~~~~~~~~~
/**
 * When the condition is true, the actuator is set to the value.
 */
export type ThenClause = {
  type: 'then';
  actuator: NominalActuator;
  actuation: Value;
};

/**
 *  Recursive type to represent a rule, can be thought of as a binary tree.
 *  Saved to the server as a JSON object.
 */
export type Rule = {
  type: 'rule';
  condition: Condition;
  then: Rule | ThenClause;
  else?: Rule | ThenClause;
};

/**
 * Condition that is valid for each input, sent down from the server to display as options.
 */
export type InputConditionConfigs = {
  input: NominalSensor | NominalActuator;
  type: PrimitiveType;
}[];

/**
 * Condition that is valid for each output, sent down from the server to display as options.
 */
export type OutputConditionConfigs = {
  input: NominalActuator;
  type: PrimitiveType;
}[];

export type CurrentSensorActuatorValues = {
  input: NominalSensor | NominalActuator;
  value: Value;
}[];
