/**
 * Lexes the input string into a nested list structure and validates the function names and number of arguments. (LISP like syntax)
 *
 * eg: '(IF,b,(EQ, x, y),d)' => ['IF', 'b', ['EQ', 'x', 'y'], 'd']
 */
const parseInputString = (input: string) => {
  const result = recursivelyParseNestedLists(input, 0);
  if (result.type === 'ERROR') {
    return result;
  }

  const { tree: listTree, index } = result;
  if (index !== input.length) {
    return {
      type: 'ERROR',
      message: `Unexpected token at index ${index}`,
      index,
    };
  }

  const tokenTree = tokenizeArguments(listTree);

  if (tokenTree.type === 'ERROR') {
    return tokenTree;
  }

  const validation = recursivelyValidateFunctions(tokenTree);
  if (validation.type === 'ERROR') {
    return validation;
  }

  return listTree;
};

console.log(
  parseInputString(
    '(IF, EQ(lightSwitch, true), SET(relay_1, true), SET(relay_1, false))',
  ),
);

/**
 * Generic error type
 */
type Err = { type: 'ERROR'; message: string; index: number };

/**
 * Types that represent the different data types that can be used in the rule engine.
 */
type DataType =
  | 'int'
  | 'float'
  | 'bool'
  | 'time'
  | 'void'
  | 'settable_int'
  | 'settable_float'
  | 'settable_bool';

/**
 * Sensors available on the device.
 */
const SENSOR_TYPES = [
  {
    name: 'temperature',
    dataType: 'float', // fahrenheit
  },
  {
    name: 'humidity',
    dataType: 'float', // percentage
  },
  {
    name: 'photoSensor',
    dataType: 'float', // percentage
  },
  {
    name: 'currentTime',
    dataType: 'time',
  },
  {
    name: 'lightSwitch',
    dataType: 'bool',
  },
] as const;

/**
 * Actuators available on the device.
 */
const ACTUATOR_TYPES = [
  {
    name: 'relay_1',
    dataType: 'settable_bool',
  },
  {
    name: 'relay_2',
    dataType: 'settable_bool',
  },
  {
    name: 'relay_3',
    dataType: 'settable_bool',
  },
  {
    name: 'relay_4',
    dataType: 'settable_bool',
  },
  {
    name: 'relay_5',
    dataType: 'settable_bool',
  },
  {
    name: 'relay_6',
    dataType: 'settable_bool',
  },
  {
    name: 'relay_7',
    dataType: 'settable_bool',
  },
  {
    name: 'relay_8',
    dataType: 'settable_bool',
  },
] as const;

type ValidationFunc = (...args: DataType[]) => boolean;

const FUNCTION_TYPES = [
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
      a !== 'void' && a === b) as ValidationFunc,
  },
  {
    name: 'NE',
    args: 2,
    returnType: 'bool',
    validateArgs: ((a: DataType, b: DataType) =>
      a !== 'void' && a === b) as ValidationFunc,
  },
  {
    name: 'GT',
    args: 2,
    returnType: 'bool',
    validateArgs: ((a: DataType, b: DataType) =>
      (a === 'float' || a === 'int') && a === b) as ValidationFunc,
  },
  {
    name: 'LT',
    args: 2,
    returnType: 'bool',
    validateArgs: ((a: DataType, b: DataType) =>
      (a === 'float' || a === 'int') && a === b) as ValidationFunc,
  },
  {
    name: 'GTE',
    args: 2,
    returnType: 'bool',
    validateArgs: ((a: DataType, b: DataType) =>
      (a === 'float' || a === 'int') && a === b) as ValidationFunc,
  },
  {
    name: 'LTE',
    args: 2,
    returnType: 'bool',
    validateArgs: ((a: DataType, b: DataType) =>
      (a === 'float' || a === 'int') && a === b) as ValidationFunc,
  },
  {
    name: 'SET',
    args: 2,
    returnType: 'void',
    validateArgs: ((actuator: DataType, val: DataType) =>
      (actuator === 'settable_bool' && val === 'bool') ||
      (actuator === 'settable_int' && val === 'int') ||
      (actuator === 'settable_float' && val === 'float')) as ValidationFunc,
  },
] as const;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~ Parse raw string into nested list structure ~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

type StringListTreeNode = (string | StringListTreeNode)[];
type NestedListResult =
  | { type: 'NestedListResult'; tree: StringListTreeNode; index: number }
  | Err;

/**
 * Recursively parses the input string into a nested list structure.
 * eg: '(a, (b, c), d)' => ['a', ['b', 'c'], 'd']
 */
const recursivelyParseNestedLists = (
  input: string,
  index: number,
): NestedListResult => {
  const tree: StringListTreeNode = [];

  if (input[index] !== '(') {
    return {
      type: 'ERROR',
      message: `Expected opening paren at index ${index}`,
      index,
    };
  }

  index++; // skip the opening paren
  let arg = ''; // current argument

  while (index < input.length) {
    const c = input[index];
    switch (c) {
      case '(':
        if (arg !== '') {
          return { type: 'ERROR', message: `Unexpected opening parens`, index };
        }
        const result = recursivelyParseNestedLists(input, index);
        if (result.type === 'ERROR') {
          return result;
        }
        const { tree: childTree, index: newIndex } = result;
        tree.push(childTree);
        index = newIndex;
        break;
      case ',':
      case ')':
        tree.push(arg);
        arg = '';
        if (c === ')')
          return { type: 'NestedListResult', tree, index: index + 1 };
        break;
      case ' ':
      case '\n':
      case '\r':
      case '\t':
        // ignore whitespace
        break;
      default:
        arg += c;
    }
    index++;
  }
  // throw new Error('Expected closing paren');
  return {
    type: 'ERROR',
    message: `Expected closing paren at index ${index}`,
    index,
  };
};

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

type TokenListTreeNode = {
  type: 'node';
  children: (Token | TokenListTreeNode)[];
};

/**
 * Converts the string list tree into a token list tree.
 *
 * 1: Tokenize the arguments
 * 2: Return the token list tree
 */
const tokenizeArguments = (
  node: StringListTreeNode,
  path = [0],
): TokenListTreeNode | Err => {
  const tokenizedArgs: (Token | TokenListTreeNode | Err)[] = node.map(
    (arg, i) => {
      if (typeof arg !== 'string') {
        return tokenizeArguments(arg, [...path, i + 1]);
      }

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
      const int = parseInt(arg);
      if (!isNaN(int)) {
        return { type: 'int', value: int } as IntToken;
      }
      const float = parseFloat(arg);
      if (!isNaN(float)) {
        return { type: 'float', value: float } as FloatToken;
      }
      if (arg === 'true' || arg === 'false') {
        return { type: 'bool', value: arg === 'true' } as BooleanToken;
      }
      if (arg.match(/^@\d{2}:\d{2}$/)) {
        return { type: 'time', value: arg } as TimeToken;
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
  if (argError.type === 'ERROR') {
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
  path = [0],
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

  const argTypes = args.map((arg) => {
    if (arg.type === 'node') {
      return recursivelyValidateFunctions(arg, path);
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
