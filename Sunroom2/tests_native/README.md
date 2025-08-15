# Native Tests

This directory contains tests that run natively on the host machine (macOS/Linux) without requiring an ESP32 board.

## Overview

Native tests are useful for testing platform-neutral code that doesn't depend on Arduino/ESP32-specific functionality. They compile and run much faster than embedded tests and can be easily integrated into CI/CD pipelines.

## Current Tests

- **`rule_core_runner.cpp`**: Tests the core rule evaluation logic in `src/rule_core.cpp`
  - Tests logical operations: EQ, AND, OR, NOT
  - Tests sensor integration with IF-SET
  - Tests composite expressions like `AND(EQ(...), NOT(...))`

## Running Tests

Use the provided script to build and run all native tests:

```bash
./run_native_tests.sh
```

This script:

1. Ensures ArduinoJson headers are available via `pio run -e native`
2. Compiles the tests using `clang++` (or `g++` on Linux)
3. Runs the compiled test binary
4. Reports results

## Adding New Tests

To add new native tests:

1. Create a new `.cpp` file in this directory
2. Include necessary headers from `src/`
3. Write test functions using simple assertions
4. Add a `main()` function to run your tests
5. Update `run_native_tests.sh` to compile and run your new test file

## Dependencies

- **ArduinoJson**: Used for JSON parsing in rule evaluation
- **Standard C++**: Tests use `std::string`, `std::function`, etc.
- **No Arduino dependencies**: Tests avoid Arduino.h, String, etc. for platform neutrality
