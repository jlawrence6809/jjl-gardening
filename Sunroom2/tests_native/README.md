# Native Tests

This directory contains tests that run natively on the host machine (macOS/Linux) without requiring an ESP32 board.

## Overview

Native tests are useful for testing platform-neutral code that doesn't depend on Arduino/ESP32-specific functionality. They compile and run much faster than embedded tests and can be easily integrated into CI/CD pipelines.

## Current Tests

- **`unified_value_tests.cpp`**: Tests the core UnifiedValue type system
  - Type construction, conversion, and validation
  - Error handling and edge cases
  - Memory management
  
- **`function_registry_tests.cpp`**: Tests the function registry system
  - Core function implementations (GT, LT, EQ, AND, OR, NOT, IF, SET, NOP)
  - Sensor function integration
  - Error handling and validation
  - Complex nested expressions

- **`bridge_integration_tests.cpp`**: Tests real-world automation scenarios
  - Temperature/humidity/light-based automation
  - Multi-sensor decision logic
  - Smart greenhouse control scenarios
  - Multiple relay control simulation

## Running Tests

### Option 1: CMake (Recommended)

```bash
# One-time setup
mkdir build && cd build
cmake ..

# Build and run all tests
make && ./native_tests

# Run specific test suites
./native_tests --gtest_filter="FunctionRegistryTest.*"
./native_tests --gtest_filter="BridgeIntegrationTest.*"

# Or use the custom target
make run_tests
```

### Option 2: Legacy Script

```bash
# From project root
./run_native_tests.sh
```

## Dependencies

- **GoogleTest**: Automatically downloaded via CMake FetchContent
- **ArduinoJson**: Provided by PlatformIO (`pio run -e native` to install)
- **Standard C++17**: Tests use modern C++ features

## Test Structure

All tests follow Google Test patterns:

```cpp
class MyTest : public ::testing::Test {
protected:
    void SetUp() override { /* setup */ }
    void TearDown() override { /* cleanup */ }
};

TEST_F(MyTest, SomeFeature) {
    EXPECT_EQ(expected, actual);
    ASSERT_TRUE(condition);
}
```

## Adding New Tests

1. Create a new `.cpp` file in this directory
2. Include necessary headers from `../src/automation_dsl/`
3. Write test functions using Google Test macros
4. Tests will be automatically discovered by CMake

## Mock System

Tests use a shared mock system defined in:
- `test_mocks.h` - Mock declarations
- `test_mocks.cpp` - Mock implementations

This provides consistent mock sensors and actuators across all test files.

## Platform Independence

Tests avoid Arduino dependencies and use:
- `std::string` instead of Arduino `String`
- `std::function` instead of Arduino function types
- Standard C++ I/O instead of `Serial`

This ensures tests run reliably on any development machine.