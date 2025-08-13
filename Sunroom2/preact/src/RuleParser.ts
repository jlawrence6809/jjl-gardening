/**
 * RuleParser.ts - LISP-like Rule Language Parser for IoT Device Control
 *
 * This module implements a parser and validator for a domain-specific language (DSL)
 * used to create automation rules for IoT devices. The language uses LISP-like syntax
 * with nested arrays to represent function calls and their arguments.
 *
 * ARCHITECTURE OVERVIEW:
 * ┌─────────────┐    ┌──────────────┐    ┌─────────────┐    ┌──────────────┐
 * │ JSON String │ -> │ Tokenization │ -> │ Validation  │ -> │ Parsed Rule  │
 * │   Input     │    │   (Syntax)   │    │ (Semantics) │    │   Output     │
 * └─────────────┘    └──────────────┘    └─────────────┘    └──────────────┘
 *
 * LANGUAGE FEATURES:
 * - Hardware sensors: temperature, humidity, photoSensor, currentTime, lightSwitch
 * - Hardware actuators: relay_1, relay_2, ..., relay_N (dynamically configured)
 * - Logical operations: AND, OR, NOT for boolean logic
 * - Comparison operations: EQ, NE, GT, LT, GTE, LTE for value comparisons
 * - Control flow: IF statements for conditional execution
 * - Actions: SET for controlling actuators
 * - Data types: int, float, bool, time, and settable variants (s_int, s_float, s_bool)
 *
 * EXAMPLE RULES:
 * Simple relay control:     ["SET", "relay_1", true]
 * Time-based automation:    ["IF", ["EQ", "currentTime", "@12:30:00"], ["SET", "relay_1", true], ["NOP"]]
 * Temperature control:      ["IF", ["GT", "temperature", 30], ["SET", "relay_1", true], ["SET", "relay_1", false]]
 * Complex logic:           ["IF", ["AND", ["GT", "temperature", 25], ["LT", "humidity", 60]], ["SET", "relay_1", true], ["NOP"]]
 *
 * VALIDATION:
 * - Syntax: Ensures proper LISP-like nested array structure
 * - Semantics: Validates function signatures and argument types
 * - Size limits: Prevents rules from exceeding embedded device memory constraints
 *
 * ERROR HANDLING:
 * All functions return either a successful result or an Err object with:
 * - Descriptive error message
 * - Path location where the error occurred
 * - Error type identification
 */

import { RELAY_COUNT } from './RelayControls';

// Maximum allowed size in bytes for a serialized rule to prevent memory issues on embedded devices
const MAX_SIZE = 256;

/**
 * Represents a parsed rule as a nested array structure (LISP-like syntax)
 * First element is always a function name, followed by its arguments
 * Arguments can be primitives or nested ParsedRule arrays
 */
export type ParsedRule = [
  keyof typeof FUNCTION_TYPES,
  ...(string | number | boolean | ParsedRule)[],
];

/**
 * Main entry point for parsing rule strings into validated rule structures.
 *
 * Takes a JSON string representing a LISP-like rule and:
 * 1. Parses the JSON into a nested array structure
 * 2. Tokenizes and validates all function names and argument types
 * 3. Ensures the rule doesn't exceed size limits for embedded devices
 *
 * @param input - JSON string representation of a rule (e.g., '["IF", ["EQ", "temperature", 25], ["SET", "relay_1", true], ["NOP"]]')
 * @returns Either a validated ParsedRule or an error object with details about what went wrong
 *
 * Example valid inputs:
 * - '["SET", "relay_1", true]' - Simple relay control
 * - '["IF", ["GT", "temperature", 30], ["SET", "relay_1", true], ["SET", "relay_1", false]]' - Conditional logic
 */
export const parseInputString = (input: string): ParsedRule | Err => {
  // Step 1: Parse the JSON string into a nested array structure
  let rule: ParsedRule;
  try {
    rule = JSON.parse(input);
  } catch (e) {
    return { type: 'ERROR', message: e.message, index: 0 };
  }

  // Step 2: Convert raw values into typed tokens and validate structure
  const tokenTree = tokenizeArguments(rule);
  if (tokenTree.type === 'ERROR') {
    return tokenTree;
  }

  // Step 3: Validate function signatures and argument types recursively
  const validation = recursivelyValidateFunctions(tokenTree);
  if (validation.type === 'ERROR') {
    return validation;
  }

  // Step 4: Check size constraints for embedded device memory limits
  const compacted = JSON.stringify(rule);
  if (compacted.length > MAX_SIZE) {
    return {
      type: 'ERROR',
      message: `Rule size exceeds the maximum size of ${MAX_SIZE} bytes: ${compacted.length}`,
      index: 0,
    };
  }

  return rule;
};

/**
 * Generic error type returned when parsing or validation fails
 * @property type - Always 'ERROR' to distinguish from successful results
 * @property message - Human-readable description of what went wrong
 * @property index - Position where the error occurred (for debugging)
 */
export type Err = { type: 'ERROR'; message: string; index: number };

/**
 * Data types supported by the rule engine
 *
 * Sensor types (read-only):
 * - 'int': Integer values (temperatures, humidity percentages, etc.)
 * - 'float': Floating-point numbers
 * - 'bool': Boolean true/false values
 * - 'time': Time values in HH:MM:SS format (prefixed with @)
 * - 'void': Functions that don't return a value (control flow)
 *
 * Actuator types (settable):
 * - 's_int': Settable integer (for numeric actuators)
 * - 's_float': Settable float (for analog outputs)
 * - 's_bool': Settable boolean (for relays, switches)
 */
type DataType =
  | 'int'
  | 'float'
  | 'bool'
  | 'time'
  | 'void'
  | 's_int'
  | 's_float'
  | 's_bool';

/**
 * Hardware sensors available on the IoT device for reading environmental data
 * Each sensor has a name that can be referenced in rules and a data type
 */
const SENSOR_TYPES = [
  {
    name: 'temperature',
    dataType: 'int', // Temperature in Celsius, range: -40 to 85°C
  },
  {
    name: 'humidity',
    dataType: 'int', // Relative humidity percentage, range: 0-100%
  },
  {
    name: 'photoSensor',
    dataType: 'int', // Light level reading, range: 0-2555 (approximate)
  },
  {
    name: 'currentTime',
    dataType: 'time', // Current time in @HH:MM:SS format
  },
  {
    name: 'lightSwitch',
    dataType: 'bool', // Physical light switch state (on/off)
  },
] as const;

/**
 * Hardware actuators (outputs) available on the device for controlling equipment
 * Dynamically generates relay definitions based on RELAY_COUNT from RelayControls
 * Each relay can be set to true (on) or false (off)
 */
const ACTUATOR_TYPES = new Array(RELAY_COUNT).fill(0).map((_, i) => ({
  name: `relay_${i + 1}`, // relay_1, relay_2, etc.
  dataType: 's_bool', // Settable boolean for on/off control
}));

// Type signature for functions that validate argument types
type ValidationFunc = (...args: DataType[]) => boolean;

/**
 * Available functions in the rule language with their signatures and validation logic
 * Each function defines:
 * - name: The function identifier used in rules
 * - args: Expected number of arguments
 * - returnType: Data type this function returns
 * - validateArgs: Function to validate argument types are compatible
 */
const FUNCTION_TYPES = [
  {
    name: 'NOP',
    args: 0,
    returnType: 'void',
    validateArgs: (() => true) as ValidationFunc, // No-operation, always valid
  },
  {
    name: 'IF', // Conditional execution: IF(condition, then_action, else_action)
    args: 3,
    returnType: 'void',
    validateArgs: ((
      conditionType: DataType,
      thenType: DataType,
      elseType: DataType,
    ) =>
      conditionType === 'bool' && // Condition must be boolean
      thenType === 'void' && // Both branches must be actions (void)
      elseType === 'void') as ValidationFunc,
  },
  {
    name: 'AND', // Logical AND: returns true if both arguments are true
    args: 2,
    returnType: 'bool',
    validateArgs: ((a: DataType, b: DataType) =>
      a === 'bool' && b === 'bool') as ValidationFunc,
  },
  {
    name: 'OR', // Logical OR: returns true if either argument is true
    args: 2,
    returnType: 'bool',
    validateArgs: ((a: DataType, b: DataType) =>
      a === 'bool' && b === 'bool') as ValidationFunc,
  },
  {
    name: 'NOT', // Logical NOT: inverts boolean value
    args: 1,
    returnType: 'bool',
    validateArgs: ((a: DataType) => a === 'bool') as ValidationFunc,
  },
  {
    name: 'EQ', // Equality comparison: returns true if arguments are equal
    args: 2,
    returnType: 'bool',
    validateArgs: ((a: DataType, b: DataType) =>
      (a === 'bool' || a === 'float' || a === 'int' || a === 'time') &&
      a === b) as ValidationFunc, // Both args must be same comparable type
  },
  {
    name: 'NE', // Not equal comparison: returns true if arguments are different
    args: 2,
    returnType: 'bool',
    validateArgs: ((a: DataType, b: DataType) =>
      (a === 'bool' || a === 'float' || a === 'int' || a === 'time') &&
      a === b) as ValidationFunc, // Both args must be same comparable type
  },
  {
    name: 'GT', // Greater than: returns true if first > second
    args: 2,
    returnType: 'bool',
    validateArgs: ((a: DataType, b: DataType) =>
      (a === 'float' || a === 'int' || a === 'time') &&
      a === b) as ValidationFunc, // Both args must be same orderable type
  },
  {
    name: 'LT', // Less than: returns true if first < second
    args: 2,
    returnType: 'bool',
    validateArgs: ((a: DataType, b: DataType) =>
      (a === 'float' || a === 'int' || a === 'time') &&
      a === b) as ValidationFunc, // Both args must be same orderable type
  },
  {
    name: 'GTE', // Greater than or equal: returns true if first >= second
    args: 2,
    returnType: 'bool',
    validateArgs: ((a: DataType, b: DataType) =>
      (a === 'float' || a === 'int' || a === 'time') &&
      a === b) as ValidationFunc, // Both args must be same orderable type
  },
  {
    name: 'LTE', // Less than or equal: returns true if first <= second
    args: 2,
    returnType: 'bool',
    validateArgs: ((a: DataType, b: DataType) =>
      (a === 'float' || a === 'int' || a === 'time') &&
      a === b) as ValidationFunc, // Both args must be same orderable type
  },
  {
    name: 'SET', // Set actuator value: SET(actuator, value)
    args: 2,
    returnType: 'void',
    validateArgs: ((actuator: DataType, val: DataType) =>
      (actuator === 's_bool' && val === 'bool') || // Set boolean actuator
      (actuator === 's_int' && val === 'int') || // Set integer actuator
      (actuator === 's_float' && val === 'float')) as ValidationFunc, // Set float actuator
  },
] as const;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~ Tokenization: Convert raw values into typed tokens ~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * Token types represent parsed and classified elements from the rule structure
 * Each token has a type field and associated data (name or value)
 */

/** Function token: represents a callable function like IF, AND, SET */
type FunctionToken = {
  type: 'function';
  name: (typeof FUNCTION_TYPES)[number]['name'];
};

/** Sensor token: represents a readable sensor like temperature, humidity */
type SensorToken = {
  type: 'sensor';
  name: (typeof SENSOR_TYPES)[number]['name'];
};

/** Actuator token: represents a controllable output like relay_1, relay_2 */
type ActuatorToken = {
  type: 'actuator';
  name: (typeof ACTUATOR_TYPES)[number]['name'];
};

/** Integer literal token: whole numbers like 25, -10, 100 */
type IntToken = {
  type: 'int';
  value: number;
};

/** Float literal token: decimal numbers like 25.5, -10.2 */
type FloatToken = {
  type: 'float';
  value: number;
};

/** Boolean literal token: true or false values */
type BooleanToken = {
  type: 'bool';
  value: boolean;
};

/** Time literal token: time strings like @12:30:00, @09:15:30 */
type TimeToken = {
  type: 'time';
  value: string;
};

/** Union of all possible token types */
type Token =
  | FunctionToken
  | SensorToken
  | ActuatorToken
  | IntToken
  | FloatToken
  | BooleanToken
  | TimeToken;

/**
 * Tree node representing a nested structure of tokens
 * Used to build the abstract syntax tree from the parsed rule
 */
export type TokenListTreeNode = {
  type: 'node';
  children: (Token | TokenListTreeNode)[];
};

/**
 * Recursively converts a parsed JSON array structure into a typed token tree
 *
 * This function takes the raw parsed JSON and identifies what each element represents:
 * - Strings are classified as functions, sensors, actuators, or time values
 * - Numbers are classified as integers or floats
 * - Booleans remain as boolean tokens
 * - Nested arrays are recursively processed
 *
 * @param node - The current node being processed (array or primitive)
 * @param path - Current path for error reporting (e.g., [0, 1, 2])
 * @returns Either a TokenListTreeNode with classified children, or an error
 */
const tokenizeArguments = (node: any, path = []): TokenListTreeNode | Err => {
  // Ensure we're processing an array (LISP-like structure requirement)
  if (!Array.isArray(node)) {
    return {
      type: 'ERROR',
      message: `Expected array at path ${path.join('.')}`,
      index: 0,
    } as Err;
  }

  // Process each element in the array, converting to appropriate token types
  const tokenizedArgs: (Token | TokenListTreeNode | Err)[] = node.map(
    (arg, i) => {
      // Nested arrays become child nodes (recursive case)
      if (Array.isArray(arg)) {
        return tokenizeArguments(arg, [...path, i]);
      }

      // String arguments: classify as function, sensor, actuator, or time
      if (typeof arg === 'string') {
        // Check if it's a known function name
        const func = FUNCTION_TYPES.find((f) => f.name === arg);
        if (func) {
          return { type: 'function', name: func.name } as FunctionToken;
        }

        // Check if it's a known sensor name
        const sensor = SENSOR_TYPES.find((s) => s.name === arg);
        if (sensor) {
          return { type: 'sensor', name: sensor.name } as SensorToken;
        }

        // Check if it's a known actuator name
        const actuator = ACTUATOR_TYPES.find((a) => a.name === arg);
        if (actuator) {
          return { type: 'actuator', name: actuator.name } as ActuatorToken;
        }

        // Check if it's a time value (format: @HH:MM:SS)
        if (arg.match(/^@\d{2}:\d{2}:\d{2}$/)) {
          return { type: 'time', value: arg } as TimeToken;
        }
      }

      // Boolean arguments become boolean tokens
      if (typeof arg === 'boolean') {
        return { type: 'bool', value: arg } as BooleanToken;
      }

      // Number arguments: distinguish between integers and floats
      if (typeof arg === 'number') {
        if (Number.isInteger(arg)) {
          return { type: 'int', value: arg } as IntToken;
        }
        return { type: 'float', value: arg } as FloatToken;
      }

      // Unrecognized argument type
      return {
        type: 'ERROR',
        message: `Unrecognized argument "${arg}" at path ${[...path, i].join(
          '.',
        )}`,
        index: i + 1,
      } as Err;
    },
  );

  // Check if any tokenization step produced an error
  const argError = tokenizedArgs.find((arg) => arg.type === 'ERROR');
  if (argError?.type === 'ERROR') {
    return argError;
  }

  // Return successful tokenization result
  return {
    type: 'node',
    children: tokenizedArgs as (Token | TokenListTreeNode)[],
  };
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~ Validation: Check function signatures and types ~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * Recursively validates function calls and their argument types throughout the token tree
 *
 * This performs semantic analysis on the parsed rule:
 * 1. Ensures each node starts with a valid function name
 * 2. Validates the correct number of arguments for each function
 * 3. Checks that argument types match function signatures
 * 4. Recursively validates nested function calls
 *
 * @param node - The token tree node to validate
 * @param path - Current path for error reporting (e.g., [0, 1])
 * @returns Either the return type of the function, or an error object
 */
const recursivelyValidateFunctions = (
  node: TokenListTreeNode,
  path = [],
): { type: DataType } | Err => {
  const pathStr = path.join('.');
  const { children } = node;
  const [funcName, ...args] = children;

  // Step 1: Ensure the first element is a function name
  if (funcName.type !== 'function') {
    return {
      type: 'ERROR',
      message: `Expected function name at path ${pathStr}`,
      index: 0,
    };
  }

  // Look up function definition to get expected signature
  const funcData = FUNCTION_TYPES.find((f) => f.name === funcName.name);

  // Step 2: Validate argument count matches function signature
  if (args.length !== funcData.args) {
    return {
      type: 'ERROR',
      message: `Expected ${funcData.args} arguments for function "${funcName.name}" at path ${pathStr}`,
      index: 0,
    };
  }

  // Step 3: Determine the data type of each argument
  const argTypes = args.map((arg, i) => {
    // Nested function call: recursively validate and get return type
    if (arg.type === 'node') {
      const result = recursivelyValidateFunctions(arg, [...path, i + 1]);
      if (result.type === 'ERROR') {
        return result; // Propagate error up
      }
      return result.type; // Return the function's return type
    }

    // Functions can't be used as arguments (they must be called)
    if (arg.type === 'function') {
      return {
        type: 'ERROR',
        message: `Unexpected function at path ${pathStr}`,
        index: 0,
      };
    }

    // Sensor reference: look up its data type
    if (arg.type === 'sensor') {
      return SENSOR_TYPES.find((s) => s.name === arg.name)?.dataType;
    }

    // Actuator reference: look up its data type
    if (arg.type === 'actuator') {
      return ACTUATOR_TYPES.find((a) => a.name === arg.name)?.dataType;
    }

    // Literal values: type matches token type
    return arg.type;
  });

  // Check if any argument type resolution failed
  const error = argTypes.find(
    (arg) => typeof arg === 'object' && arg.type === 'ERROR',
  ) as Err | undefined;
  if (error) {
    return error;
  }

  // Step 4: Use function's custom validation to check argument type compatibility
  const validation = funcData.validateArgs(...(argTypes as DataType[]));

  if (!validation) {
    return {
      type: 'ERROR',
      message: `Invalid argument types for function "${funcName.name}" at path ${pathStr}`,
      index: 0,
    };
  }

  // Return the data type this function produces
  return { type: funcData.returnType };
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~ Development & Testing ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * COMMENTED OUT: Optional compactification function for reducing rule size
 *
 * This function could be used to minimize the serialized rule size by:
 * - Converting function/sensor/actuator tokens to just their names
 * - Shortening boolean values to '1'/'0'
 * - Removing unnecessary whitespace
 *
 * Currently not used as JSON.stringify() provides sufficient size control
 */
// const compactify = (node: TokenListTreeNode) => {
//   return node.children.map((child) => {
//     if (child.type === 'node') {
//       return compactify(child);
//     }
//     if (child.type === 'function') {
//       return child.name;
//     }
//     if (child.type === 'sensor' || child.type === 'actuator') {
//       return child.name;
//     }
//     if (child.type === 'float') {
//       return `${child.value.toFixed()}f`;
//     }
//     if (child.type === 'bool') {
//       return child.value ? '1' : '0';
//     }
//     return child.value;
//   });
// };

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Examples ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * EXAMPLE USAGE (commented out for production):
 *
 * Time-based relay control:
 * parseInputString('["IF", ["EQ", "currentTime", "@12:30:00"], ["SET", "relay_1", true], ["SET", "relay_1", false]]')
 *
 * Light switch triggered relay:
 * parseInputString('["IF", ["EQ", "lightSwitch", true], ["SET", "relay_1", true], ["SET", "relay_1", false]]')
 */

// console.log(
//   parseInputString(
//     '["IF", ["EQ", "currentTime", "@12:30:00"], ["SET", "relay_1", true], ["SET", "relay_1", false]]',
//   ),
// );

// console.log(
//   parseInputString(
//     '["IF", ["EQ", "lightSwitch", true], ["SET", "relay_1", true], ["SET", "relay_1", false]]',
//   ),
// );

/**
 * EXAMPLE: Rule that exceeds size limit (nested IF statements)
 * This demonstrates the MAX_SIZE validation in action - deeply nested rules
 * like this would be rejected to prevent memory issues on embedded devices.
 *
 * The example shows multiple levels of conditional logic that, while syntactically
 * valid, would create a rule larger than the 256-byte limit.
 */
/*
[
    "IF",
    ["EQ", "currentTime", "@12:30:00"],
    [
        "IF",
        ["EQ", "currentTime", "@12:30:00"],
        [
            "IF",
            ["EQ", "currentTime", "@12:30:00"],
            ["SET", "relay_1", true],
            ["SET", "relay_1", false]
        ],
        [
            "IF",
            ["EQ", "currentTime", "@12:30:00"],
            ["SET", "relay_1", true],
            ["SET", "relay_1", false]
        ]
    ],
    [
        "IF",
        ["EQ", "currentTime", "@12:30:00"],
        [
            "IF",
            ["EQ", "currentTime", "@12:30:00"],
            ["SET", "relay_1", true],
            ["SET", "relay_1", false]
        ],
        [
            "IF",
            ["EQ", "currentTime", "@12:30:00"],
            ["SET", "relay_1", true],
            ["SET", "relay_1", false]
        ]
    ]
]
*/
