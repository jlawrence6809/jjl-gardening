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

type ValidationFunc = (...args: DataType[]) => boolean;

const FUNCTION_TYPES = [
  {
    name: 'NOP',
    args: 0,
    returnType: 'void',
    validateArgs: (() => true) as ValidationFunc,
  },
  {
    name: 'IF',
    args: 3,
    returnType: 'void',
    validateArgs: ((
      conditionType: DataType,
      thenType: DataType,
      elseType: DataType,
    ) =>
      conditionType === 'bool' &&
      thenType === 'void' &&
      elseType === 'void') as ValidationFunc,
  },
  {
    name: 'AND',
    args: 2,
    returnType: 'bool',
    validateArgs: ((a: DataType, b: DataType) =>
      a === 'bool' && b === 'bool') as ValidationFunc,
  },
  {
    name: 'OR',
    args: 2,
    returnType: 'bool',
    validateArgs: ((a: DataType, b: DataType) =>
      a === 'bool' && b === 'bool') as ValidationFunc,
  },
  {
    name: 'NOT',
    args: 1,
    returnType: 'bool',
    validateArgs: ((a: DataType) => a === 'bool') as ValidationFunc,
  },
  {
    name: 'EQ',
    args: 2,
    returnType: 'bool',
    validateArgs: ((a: DataType, b: DataType) =>
      (a === 'bool' || a === 'float' || a === 'int' || a === 'time') &&
      a === b) as ValidationFunc,
  },
  {
    name: 'NE',
    args: 2,
    returnType: 'bool',
    validateArgs: ((a: DataType, b: DataType) =>
      (a === 'bool' || a === 'float' || a === 'int' || a === 'time') &&
      a === b) as ValidationFunc,
  },
  {
    name: 'GT',
    args: 2,
    returnType: 'bool',
    validateArgs: ((a: DataType, b: DataType) =>
      (a === 'float' || a === 'int' || a === 'time') &&
      a === b) as ValidationFunc,
  },
  {
    name: 'LT',
    args: 2,
    returnType: 'bool',
    validateArgs: ((a: DataType, b: DataType) =>
      (a === 'float' || a === 'int' || a === 'time') &&
      a === b) as ValidationFunc,
  },
  {
    name: 'GTE',
    args: 2,
    returnType: 'bool',
    validateArgs: ((a: DataType, b: DataType) =>
      (a === 'float' || a === 'int' || a === 'time') &&
      a === b) as ValidationFunc,
  },
  {
    name: 'LTE',
    args: 2,
    returnType: 'bool',
    validateArgs: ((a: DataType, b: DataType) =>
      (a === 'float' || a === 'int' || a === 'time') &&
      a === b) as ValidationFunc,
  },
  {
    name: 'SET',
    args: 2,
    returnType: 'void',
    validateArgs: ((actuator: DataType, val: DataType) =>
      (actuator === 's_bool' && val === 'bool') ||
      (actuator === 's_int' && val === 'int') ||
      (actuator === 's_float' && val === 'float')) as ValidationFunc,
  },
] as const;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~ Parse raw string into nested list structure ~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

type FunctionToken = {
  type: 'function';
  name: (typeof FUNCTION_TYPES)[number]['name'];
};

type SensorToken = {
  type: 'sensor';
  name: (typeof SENSOR_TYPES)[number]['name'];
};

type ActuatorToken = {
  type: 'actuator';
  name: (typeof ACTUATOR_TYPES)[number]['name'];
};

type IntToken = {
  type: 'int';
  value: number;
};

type FloatToken = {
  type: 'float';
  value: number;
};

type BooleanToken = {
  type: 'bool';
  value: boolean;
};

type TimeToken = {
  type: 'time';
  value: string;
};

type Token =
  | FunctionToken
  | SensorToken
  | ActuatorToken
  | IntToken
  | FloatToken
  | BooleanToken
  | TimeToken;

export type TokenListTreeNode = {
  type: 'node';
  children: (Token | TokenListTreeNode)[];
};

/**
 * Converts the string list tree into a token list tree.
 *
 * 1: Tokenize the arguments
 * 2: Return the token list tree
 */
const tokenizeArguments = (node: any, path = []): TokenListTreeNode | Err => {
  if (!Array.isArray(node)) {
    return {
      type: 'ERROR',
      message: `Expected array at path ${path.join('.')}`,
      index: 0,
    } as Err;
  }

  const tokenizedArgs: (Token | TokenListTreeNode | Err)[] = node.map(
    (arg, i) => {
      if (Array.isArray(arg)) {
        return tokenizeArguments(arg, [...path, i]);
      }

      if (typeof arg === 'string') {
        const func = FUNCTION_TYPES.find((f) => f.name === arg);
        if (func) {
          return { type: 'function', name: func.name } as FunctionToken;
        }
        const sensor = SENSOR_TYPES.find((s) => s.name === arg);
        if (sensor) {
          return { type: 'sensor', name: sensor.name } as SensorToken;
        }
        const actuator = ACTUATOR_TYPES.find((a) => a.name === arg);
        if (actuator) {
          return { type: 'actuator', name: actuator.name } as ActuatorToken;
        }
        if (arg.match(/^@\d{2}:\d{2}:\d{2}$/)) {
          return { type: 'time', value: arg } as TimeToken;
        }
      }
      if (typeof arg === 'boolean') {
        return { type: 'bool', value: arg } as BooleanToken;
      }

      if (typeof arg === 'number') {
        if (Number.isInteger(arg)) {
          return { type: 'int', value: arg } as IntToken;
        }
        return { type: 'float', value: arg } as FloatToken;
      }

      return {
        type: 'ERROR',
        message: `Unrecognized argument "${arg}" at path ${[...path, i].join(
          '.',
        )}`,
        index: i + 1,
      } as Err;
    },
  );
  const argError = tokenizedArgs.find((arg) => arg.type === 'ERROR');
  if (argError?.type === 'ERROR') {
    return argError;
  }

  return {
    type: 'node',
    children: tokenizedArgs as (Token | TokenListTreeNode)[],
  };
};

/**
 * Recursively validates the function names and number of arguments.
 * Returns true if the function is valid, or an error object if invalid.
 *
 * 1: Validate the function name
 * 2: Validate the number of arguments
 * 3: Validate the argument types
 * 4: Return the return type of the function
 */
const recursivelyValidateFunctions = (
  node: TokenListTreeNode,
  path = [],
): { type: DataType } | Err => {
  const pathStr = path.join('.');
  const { children } = node;
  const [funcName, ...args] = children;

  if (funcName.type !== 'function') {
    return {
      type: 'ERROR',
      message: `Expected function name at path ${pathStr}`,
      index: 0,
    };
  }

  const funcData = FUNCTION_TYPES.find((f) => f.name === funcName.name);

  if (args.length !== funcData.args) {
    return {
      type: 'ERROR',
      message: `Expected ${funcData.args} arguments for function "${funcName.name}" at path ${pathStr}`,
      index: 0,
    };
  }

  const argTypes = args.map((arg, i) => {
    if (arg.type === 'node') {
      const result = recursivelyValidateFunctions(arg, [...path, i + 1]);
      if (result.type === 'ERROR') {
        return result;
      }
      return result.type;
    }
    if (arg.type === 'function') {
      return {
        type: 'ERROR',
        message: `Unexpected function at path ${pathStr}`,
        index: 0,
      };
    }

    if (arg.type === 'sensor') {
      return SENSOR_TYPES.find((s) => s.name === arg.name)?.dataType;
    }

    if (arg.type === 'actuator') {
      return ACTUATOR_TYPES.find((a) => a.name === arg.name)?.dataType;
    }

    return arg.type;
  });

  const error = argTypes.find(
    (arg) => typeof arg === 'object' && arg.type === 'ERROR',
  ) as Err | undefined;
  if (error) {
    return error;
  }

  const validation = funcData.validateArgs(...(argTypes as DataType[]));

  if (!validation) {
    return {
      type: 'ERROR',
      message: `Invalid argument types for function "${funcName.name}" at path ${pathStr}`,
      index: 0,
    };
  }

  return { type: funcData.returnType };
};

/**
 * Shrink down keys and values as much as possible.
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

/*
Too large example:

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
