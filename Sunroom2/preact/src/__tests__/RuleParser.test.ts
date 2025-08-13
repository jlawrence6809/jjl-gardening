// Mock the RELAY_COUNT import to avoid importing index.tsx
jest.mock('../index', () => ({
  RELAY_COUNT: 8,
}));

import { parseInputString, ParsedRule, Err } from '../RuleParser';

describe('RuleParser', () => {
  describe('parseInputString', () => {
    describe('valid rules', () => {
      test('should parse simple IF rule with temperature comparison', () => {
        const input =
          '["IF", ["GT", "temperature", 25], ["SET", "relay_1", true], ["SET", "relay_1", false]]';
        const result = parseInputString(input);

        expect(result).not.toHaveProperty('type', 'ERROR');
        expect(Array.isArray(result)).toBe(true);
        if (Array.isArray(result)) {
          expect(result[0]).toBe('IF');
          expect(result[1]).toEqual(['GT', 'temperature', 25]);
          expect(result[2]).toEqual(['SET', 'relay_1', true]);
          expect(result[3]).toEqual(['SET', 'relay_1', false]);
        }
      });

      test('should parse time-based rule', () => {
        const input =
          '["IF", ["EQ", "currentTime", "@12:30:00"], ["SET", "relay_1", true], ["NOP"]]';
        const result = parseInputString(input);

        expect(result).not.toHaveProperty('type', 'ERROR');
        expect(Array.isArray(result)).toBe(true);
        if (Array.isArray(result)) {
          expect(result[0]).toBe('IF');
          expect(result[1]).toEqual(['EQ', 'currentTime', '@12:30:00']);
          expect(result[2]).toEqual(['SET', 'relay_1', true]);
          expect(result[3]).toEqual(['NOP']);
        }
      });

      test('should parse complex logical rule with AND', () => {
        const input =
          '["IF", ["AND", ["GT", "temperature", 20], ["LT", "humidity", 60]], ["SET", "relay_2", true], ["SET", "relay_2", false]]';
        const result = parseInputString(input);

        expect(result).not.toHaveProperty('type', 'ERROR');
        expect(Array.isArray(result)).toBe(true);
        if (Array.isArray(result)) {
          expect(result[0]).toBe('IF');
          expect(result[1]).toEqual([
            'AND',
            ['GT', 'temperature', 20],
            ['LT', 'humidity', 60],
          ]);
        }
      });

      test('should parse rule with OR logic', () => {
        const input =
          '["IF", ["OR", ["EQ", "lightSwitch", true], ["GT", "photoSensor", 1000]], ["SET", "relay_3", true], ["NOP"]]';
        const result = parseInputString(input);

        expect(result).not.toHaveProperty('type', 'ERROR');
        expect(Array.isArray(result)).toBe(true);
      });

      test('should parse rule with NOT logic', () => {
        const input =
          '["IF", ["NOT", ["EQ", "lightSwitch", true]], ["SET", "relay_1", false], ["NOP"]]';
        const result = parseInputString(input);

        expect(result).not.toHaveProperty('type', 'ERROR');
        expect(Array.isArray(result)).toBe(true);
      });

      test('should return error when comparing int sensor with float value', () => {
        const input =
          '["IF", ["GT", "temperature", 25.5], ["SET", "relay_1", true], ["NOP"]]';
        const result = parseInputString(input) as Err;

        expect(result.type).toBe('ERROR');
        expect(result.message).toContain(
          'Invalid argument types for function "GT"',
        );
      });

      test('should parse rule with different comparison operators', () => {
        const operators = ['EQ', 'NE', 'GT', 'LT', 'GTE', 'LTE'];

        operators.forEach(op => {
          const input = `["IF", ["${op}", "temperature", 25], ["SET", "relay_1", true], ["NOP"]]`;
          const result = parseInputString(input);

          expect(result).not.toHaveProperty('type', 'ERROR');
          expect(Array.isArray(result)).toBe(true);
        });
      });

      test('should parse NOP (no operation) rule', () => {
        const input = '["NOP"]';
        const result = parseInputString(input);

        expect(result).not.toHaveProperty('type', 'ERROR');
        expect(Array.isArray(result)).toBe(true);
        if (Array.isArray(result)) {
          expect(result[0]).toBe('NOP');
        }
      });
    });

    describe('invalid JSON input', () => {
      test('should return error for malformed JSON', () => {
        const input =
          '["IF", ["GT", "temperature", 25], ["SET", "relay_1", true]'; // Missing closing bracket
        const result = parseInputString(input) as Err;

        expect(result.type).toBe('ERROR');
        expect(result.message).toContain('JSON');
        expect(result.index).toBe(0);
      });

      test('should return error for invalid JSON syntax', () => {
        const input = '{"invalid": "json", "for": "rule"}'; // Object instead of array
        const result = parseInputString(input) as Err;

        expect(result.type).toBe('ERROR');
        expect(result.message).toContain('Expected array');
      });

      test('should return error for empty string', () => {
        const input = '';
        const result = parseInputString(input) as Err;

        expect(result.type).toBe('ERROR');
        expect(result.message).toContain('Unexpected end of JSON input');
      });
    });

    describe('tokenization errors', () => {
      test('should return error for unrecognized function', () => {
        const input = '["INVALID_FUNC", "temperature", 25]';
        const result = parseInputString(input) as Err;

        expect(result.type).toBe('ERROR');
        expect(result.message).toContain(
          'Unrecognized argument "INVALID_FUNC"',
        );
      });

      test('should return error for unrecognized sensor', () => {
        const input =
          '["IF", ["GT", "invalidSensor", 25], ["SET", "relay_1", true], ["NOP"]]';
        const result = parseInputString(input) as Err;

        expect(result.type).toBe('ERROR');
        expect(result.message).toContain(
          'Unrecognized argument "invalidSensor"',
        );
      });

      test('should return error for unrecognized actuator', () => {
        const input =
          '["IF", ["GT", "temperature", 25], ["SET", "invalid_relay", true], ["NOP"]]';
        const result = parseInputString(input) as Err;

        expect(result.type).toBe('ERROR');
        expect(result.message).toContain(
          'Unrecognized argument "invalid_relay"',
        );
      });

      test('should accept time format that matches regex pattern', () => {
        // The regex only checks format, not validity of time values
        const input =
          '["IF", ["EQ", "currentTime", "@25:70:80"], ["SET", "relay_1", true], ["NOP"]]';
        const result = parseInputString(input);

        expect(result).not.toHaveProperty('type', 'ERROR');
      });

      test('should return error for time format that does not match regex', () => {
        const input =
          '["IF", ["EQ", "currentTime", "@1:2:3"], ["SET", "relay_1", true], ["NOP"]]'; // Single digits
        const result = parseInputString(input) as Err;

        expect(result.type).toBe('ERROR');
        expect(result.message).toContain('Unrecognized argument "@1:2:3"');
      });

      test('should accept valid time format', () => {
        const input =
          '["IF", ["EQ", "currentTime", "@23:59:59"], ["SET", "relay_1", true], ["NOP"]]';
        const result = parseInputString(input);

        expect(result).not.toHaveProperty('type', 'ERROR');
      });
    });

    describe('validation errors', () => {
      test('should return error for wrong number of arguments in IF', () => {
        const input =
          '["IF", ["GT", "temperature", 25], ["SET", "relay_1", true]]'; // Missing else clause
        const result = parseInputString(input) as Err;

        expect(result.type).toBe('ERROR');
        expect(result.message).toContain(
          'Expected 3 arguments for function "IF"',
        );
      });

      test('should return error for wrong number of arguments in GT', () => {
        const input =
          '["IF", ["GT", "temperature"], ["SET", "relay_1", true], ["NOP"]]'; // Missing second argument
        const result = parseInputString(input) as Err;

        expect(result.type).toBe('ERROR');
        expect(result.message).toContain(
          'Expected 2 arguments for function "GT"',
        );
      });

      test('should return error for wrong argument types in AND', () => {
        const input =
          '["IF", ["AND", "temperature", 25], ["SET", "relay_1", true], ["NOP"]]'; // temperature is not bool
        const result = parseInputString(input) as Err;

        expect(result.type).toBe('ERROR');
        expect(result.message).toContain(
          'Invalid argument types for function "AND"',
        );
      });

      test('should return error for wrong argument types in SET', () => {
        const input =
          '["IF", ["GT", "temperature", 25], ["SET", "temperature", true], ["NOP"]]'; // Can't set sensor
        const result = parseInputString(input) as Err;

        expect(result.type).toBe('ERROR');
        expect(result.message).toContain(
          'Invalid argument types for function "SET"',
        );
      });

      test('should return error for type mismatch in comparison', () => {
        const input =
          '["IF", ["GT", "temperature", true], ["SET", "relay_1", true], ["NOP"]]'; // Can't compare int to bool
        const result = parseInputString(input) as Err;

        expect(result.type).toBe('ERROR');
        expect(result.message).toContain(
          'Invalid argument types for function "GT"',
        );
      });

      test('should return error for comparing different numeric types', () => {
        const input =
          '["IF", ["EQ", "temperature", "@12:00:00"], ["SET", "relay_1", true], ["NOP"]]'; // Can't compare int to time
        const result = parseInputString(input) as Err;

        expect(result.type).toBe('ERROR');
        expect(result.message).toContain(
          'Invalid argument types for function "EQ"',
        );
      });
    });

    describe('size limits', () => {
      test('should return error for rule exceeding maximum size', () => {
        // Create a very large nested rule that exceeds 256 bytes
        const largeRule = [
          'IF',
          ['EQ', 'currentTime', '@12:30:00'],
          [
            'IF',
            ['EQ', 'currentTime', '@12:30:00'],
            [
              'IF',
              ['EQ', 'currentTime', '@12:30:00'],
              ['SET', 'relay_1', true],
              ['SET', 'relay_1', false],
            ],
            [
              'IF',
              ['EQ', 'currentTime', '@12:30:00'],
              ['SET', 'relay_1', true],
              ['SET', 'relay_1', false],
            ],
          ],
          [
            'IF',
            ['EQ', 'currentTime', '@12:30:00'],
            [
              'IF',
              ['EQ', 'currentTime', '@12:30:00'],
              ['SET', 'relay_1', true],
              ['SET', 'relay_1', false],
            ],
            [
              'IF',
              ['EQ', 'currentTime', '@12:30:00'],
              ['SET', 'relay_1', true],
              ['SET', 'relay_1', false],
            ],
          ],
        ];

        const input = JSON.stringify(largeRule);
        const result = parseInputString(input) as Err;

        expect(result.type).toBe('ERROR');
        expect(result.message).toContain(
          'Rule size exceeds the maximum size of 256 bytes',
        );
      });

      test('should accept rule within size limits', () => {
        const input =
          '["IF", ["GT", "temperature", 25], ["SET", "relay_1", true], ["NOP"]]';
        const result = parseInputString(input);

        expect(result).not.toHaveProperty('type', 'ERROR');
        expect(JSON.stringify(result).length).toBeLessThanOrEqual(256);
      });
    });

    describe('nested structures', () => {
      test('should handle deeply nested logical operations', () => {
        const input =
          '["IF", ["AND", ["OR", ["GT", "temperature", 20], ["LT", "humidity", 50]], ["NOT", ["EQ", "lightSwitch", false]]], ["SET", "relay_1", true], ["NOP"]]';
        const result = parseInputString(input);

        expect(result).not.toHaveProperty('type', 'ERROR');
        expect(Array.isArray(result)).toBe(true);
      });

      test('should handle nested IF statements', () => {
        const input =
          '["IF", ["GT", "temperature", 25], ["IF", ["LT", "humidity", 60], ["SET", "relay_1", true], ["NOP"]], ["NOP"]]';
        const result = parseInputString(input);

        expect(result).not.toHaveProperty('type', 'ERROR');
        expect(Array.isArray(result)).toBe(true);
      });
    });

    describe('sensor and actuator validation', () => {
      test('should accept all valid sensors', () => {
        const sensors = [
          'temperature',
          'humidity',
          'photoSensor',
          'currentTime',
          'lightSwitch',
        ];

        sensors.forEach(sensor => {
          let input: string;
          if (sensor === 'currentTime') {
            input = `["IF", ["EQ", "${sensor}", "@12:00:00"], ["SET", "relay_1", true], ["NOP"]]`;
          } else if (sensor === 'lightSwitch') {
            input = `["IF", ["EQ", "${sensor}", true], ["SET", "relay_1", true], ["NOP"]]`;
          } else {
            input = `["IF", ["GT", "${sensor}", 25], ["SET", "relay_1", true], ["NOP"]]`;
          }

          const result = parseInputString(input);
          expect(result).not.toHaveProperty('type', 'ERROR');
        });
      });

      test('should accept valid relay actuators', () => {
        // Test multiple relay numbers (assuming RELAY_COUNT is at least 3)
        const relays = ['relay_1', 'relay_2', 'relay_3'];

        relays.forEach(relay => {
          const input = `["IF", ["GT", "temperature", 25], ["SET", "${relay}", true], ["NOP"]]`;
          const result = parseInputString(input);

          expect(result).not.toHaveProperty('type', 'ERROR');
        });
      });
    });

    describe('boolean and numeric values', () => {
      test('should handle boolean true and false', () => {
        const inputTrue =
          '["IF", ["EQ", "lightSwitch", true], ["SET", "relay_1", true], ["NOP"]]';
        const inputFalse =
          '["IF", ["EQ", "lightSwitch", false], ["SET", "relay_1", false], ["NOP"]]';

        expect(parseInputString(inputTrue)).not.toHaveProperty('type', 'ERROR');
        expect(parseInputString(inputFalse)).not.toHaveProperty(
          'type',
          'ERROR',
        );
      });

      test('should handle integer values', () => {
        const input =
          '["IF", ["EQ", "temperature", 25], ["SET", "relay_1", true], ["NOP"]]';
        const result = parseInputString(input);

        expect(result).not.toHaveProperty('type', 'ERROR');
      });

      test('should return error when comparing int sensor with float values', () => {
        const input =
          '["IF", ["GT", "temperature", 25.7], ["SET", "relay_1", true], ["NOP"]]';
        const result = parseInputString(input) as Err;

        expect(result.type).toBe('ERROR');
        expect(result.message).toContain(
          'Invalid argument types for function "GT"',
        );
      });

      test('should handle float values in literal comparisons', () => {
        // Test that float tokenization works by using EQ with two floats
        const input =
          '["IF", ["EQ", 25.5, 25.5], ["SET", "relay_1", true], ["NOP"]]';
        const result = parseInputString(input);

        // This should work - the parser accepts literal float comparisons
        expect(result).not.toHaveProperty('type', 'ERROR');
        expect(Array.isArray(result)).toBe(true);
      });

      test('should handle negative numbers', () => {
        const input =
          '["IF", ["GT", "temperature", -10], ["SET", "relay_1", true], ["NOP"]]';
        const result = parseInputString(input);

        expect(result).not.toHaveProperty('type', 'ERROR');
      });
    });
  });
});
