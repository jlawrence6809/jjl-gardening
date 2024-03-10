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

export type BooleanComparison = '==' | '!=';
export type NumberComparison = '==' | '!=' | '>' | '<';

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

// ~~~~~~~~~~~~~ RULE CODE BREAKDOWN ~~~~~~~~~~~~~

// type Token = { type: string; value?: string };

type PAREN = { type: 'PAREN'; value: '(' | ')' };
type AND = { type: 'AND' };
type OR = { type: 'OR' };
type NOT = { type: 'NOT' };
type COMPARISON = { type: 'COMPARISON'; value: string };
type ASSIGNMENT = { type: 'ASSIGNMENT' };
type IDENTIFIER = { type: 'IDENTIFIER'; value: string };
type NUMBER = { type: 'NUMBER'; value: string };
type HOUR_MINUTE = { type: 'HOUR_MINUTE'; value: string };
type ELSE = { type: 'ELSE' };
type BOOLEAN = { type: 'BOOLEAN'; value: 'true' | 'false' };
type THEN = { type: 'THEN' };

type Token =
  | PAREN
  | AND
  | OR
  | NOT
  | COMPARISON
  | ASSIGNMENT
  | IDENTIFIER
  | NUMBER
  | HOUR_MINUTE
  | ELSE
  | BOOLEAN
  | THEN;

/**
 * Tokenizes the input string into an array of tokens.
 *
 * Example:
 * tokenize('temperature > 30 & humidity < 50 | (light > 100 & light < 200) & timestamp > @12:00 | alwaysOn == true ? led = true : fan = false')
 *
 */
function tokenize(input: string): Token[] {
  input = input.replace(/\s/g, ''); // Remove whitespace

  const isNumber = (c) => !isNaN(Number(c));
  const isAlpha = (c) => /^[A-Za-z]$/.test(c);

  const tokens: Token[] = [];
  let currentIndex = 0;

  let safetyMax = 1000;

  while (currentIndex < input.length && safetyMax-- > 0) {
    let char = input[currentIndex];
    const lastToken = tokens[tokens.length - 1];

    if (char === '(' || char === ')') {
      tokens.push({ type: 'PAREN', value: char });
      currentIndex++;
    } else if (char === '=') {
      if (lastToken?.type === 'COMPARISON') {
        lastToken.value += char;
        currentIndex++;
      } else if (input[currentIndex + 1] === '=') {
        tokens.push({ type: 'COMPARISON', value: '==' });
        currentIndex += 2;
      } else {
        tokens.push({ type: 'ASSIGNMENT' });
        currentIndex++;
      }
    } else if (char === '&') {
      tokens.push({ type: 'AND' });
      currentIndex++;
    } else if (char === '|') {
      tokens.push({ type: 'OR' });
      currentIndex++;
    } else if (char === '!') {
      if (input[currentIndex + 1] === '=') {
        tokens.push({ type: 'COMPARISON', value: '!=' });
        currentIndex += 2;
      } else {
        tokens.push({ type: 'NOT' });
        currentIndex++;
      }
    } else if (char === '<' || char === '>') {
      tokens.push({ type: 'COMPARISON', value: char });
      currentIndex++;
    } else if (isNumber(char) || char === '-') {
      let number = char;
      currentIndex++;
      while (
        safetyMax-- > 0 &&
        (isNumber(input[currentIndex]) ||
          (!number.includes('.') && input[currentIndex] === '.'))
      ) {
        number += input[currentIndex];
        currentIndex++;
      }
      if (
        lastToken?.type === 'HOUR_MINUTE' &&
        !/[0-9]+\:[0-9]+/.test(lastToken.value)
      ) {
        lastToken.value += number;
      } else {
        tokens.push({ type: 'NUMBER', value: number });
      }
    } else if (char === '@') {
      tokens.push({ type: 'HOUR_MINUTE', value: '' });
      currentIndex++;
    } else if (char === ':') {
      if (lastToken?.type === 'HOUR_MINUTE' && !lastToken.value.includes(':')) {
        lastToken.value += char;
      } else {
        tokens.push({ type: 'ELSE' });
      }
      currentIndex++;
    } else if (
      char === 't' &&
      input[currentIndex + 1] === 'r' &&
      input[currentIndex + 2] === 'u' &&
      input[currentIndex + 3] === 'e'
    ) {
      tokens.push({ type: 'BOOLEAN', value: 'true' });
      currentIndex += 4;
    } else if (
      char === 'f' &&
      input[currentIndex + 1] === 'a' &&
      input[currentIndex + 2] === 'l' &&
      input[currentIndex + 3] === 's' &&
      input[currentIndex + 4] === 'e'
    ) {
      tokens.push({ type: 'BOOLEAN', value: 'false' });
      currentIndex += 5;
    } else if (char === '?') {
      tokens.push({ type: 'THEN' });
      currentIndex++;
    } else if (isAlpha(char)) {
      if (lastToken?.type === 'IDENTIFIER') {
        lastToken.value += char;
      } else {
        tokens.push({ type: 'IDENTIFIER', value: char });
      }
      currentIndex++;
    } else {
      throw new Error('Unrecognized token');
    }
  }

  return tokens;
}

const parseTokens = (tokens: Token[]) /*: Rule*/ => {
  console.log('hello');

  type ParsedTernaryToken = {
    type: 'rule';
    condition: Token[];
    thenClause: (ParsedTernaryToken | Token)[];
    elseClause: (ParsedTernaryToken | Token)[];
    tokenIdx: number;
  };

  /**
   * Recursively parses the tokens into a tree structure.
   *
   * iterates over tokens starting from tokenIdx until a "?" is found or the end of the tokens array, add the tokens to condition
   * iterates over tokens starting from tokenIdx until a ":" is found (if a "?" is found then recurse) or the end of the tokens array, add the tokens to thenClause
   * iterates over tokens starting from tokenIdx until a ":" is found (indicating we're within a parent ternary) or the end of the tokens array, add the tokens to elseClause
   */
  const recurseForTernary = (tokenIdx: number): ParsedTernaryToken => {
    let condition: Token[] = [];
    let thenClause: (ParsedTernaryToken | Token)[] = [];
    let elseClause: (ParsedTernaryToken | Token)[] = [];
    let maxIter = 1000;

    const nextToken = (): Token | undefined => {
      return tokens[tokenIdx++];
    };

    const peekToken = (): Token | undefined => {
      return tokens[tokenIdx];
    };

    // let currentToken: Token | undefined = peekToken();
    while (maxIter-- > 0 && peekToken() && peekToken()?.type !== 'THEN') {
      condition.push(nextToken());
    }
    nextToken(); // consume the THEN token to prevent iterateUntilElse from recursing into the thenClause immediately

    /**
     * Iterates over tokens starting from tokenIdx until a ":" is found (if a "?" is found then recurse) or the end of the tokens array, add the tokens to thenClause
     */
    const iterateUntilElse = () => {
      const startingTokenIdx = tokenIdx;
      let clause: (ParsedTernaryToken | Token)[] = [];

      while (maxIter-- > 0 && peekToken() && peekToken()?.type !== 'ELSE') {
        if (peekToken()?.type === 'THEN') {
          // Recurse into the thenClause and return the result, note that we don't consume the THEN token here because it was already consumed in the recursive call
          const ternary = recurseForTernary(startingTokenIdx);
          tokenIdx = ternary.tokenIdx;
          return [ternary];
        }
        clause.push(nextToken());
      }
      nextToken(); // Consume the else
      return clause;
    };

    thenClause = iterateUntilElse();
    elseClause = iterateUntilElse();

    return {
      type: 'rule',
      condition,
      thenClause,
      elseClause,
      tokenIdx,
    };
  };

  const ternaries = recurseForTernary(0);
};

// type ASTNode = { type: string; left?: ASTNode; right?: ASTNode; value?: any };

// function parse(tokens: Token[]): ASTNode {
//   let currentTokenIndex = 0;

//   function parseExpression(): ASTNode {
//     // Example parsing logic for an expression
//     // This is a simplified placeholder; actual logic will depend on your grammar
//     const token = tokens[currentTokenIndex++];
//     if (token.type === 'NUMBER') {
//       return { type: 'Literal', value: token.value };
//     }

//     // Parse other types of expressions (binary operations, etc.)

//     throw new Error('Unrecognized token type');
//   }

//   return parseExpression();
// }

class TrieNode {
  children: { [key: string]: TrieNode } = {};
  isEndOfWord: boolean = false;
}

class AutocompleteSystem {
  root: TrieNode = new TrieNode();

  insert(word: string) {
    let node = this.root;
    for (let char of word) {
      if (!node.children[char]) {
        node.children[char] = new TrieNode();
      }
      node = node.children[char];
    }
    node.isEndOfWord = true;
  }

  search(prefix: string): string[] {
    let node = this.root;
    for (let char of prefix) {
      if (!node.children[char]) {
        return [];
      }
      node = node.children[char];
    }

    // Perform DFS to collect all words that start with the prefix
    return this.collectWords(node, prefix);
  }

  collectWords(node: TrieNode, prefix: string): string[] {
    let words: string[] = [];
    if (node.isEndOfWord) {
      words.push(prefix);
    }

    for (let char in node.children) {
      words = words.concat(
        this.collectWords(node.children[char], prefix + char),
      );
    }

    return words;
  }
}
