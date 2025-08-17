cmake_minimum_required(VERSION 3.16)
project(Sunroom2Tests)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find GoogleTest (prefer system installation, fallback to FetchContent)
find_package(GTest QUIET)

if(NOT GTest_FOUND)
    message(STATUS "GTest not found system-wide, trying FetchContent...")
    include(FetchContent)
    
    FetchContent_Declare(
      googletest
      GIT_REPOSITORY https://github.com/google/googletest.git
      GIT_TAG v1.17.0  # Use a stable tag instead of commit hash
    )
    
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    
    FetchContent_MakeAvailable(googletest)
    
    # Create aliases for consistency
    add_library(GTest::gtest ALIAS gtest)
    add_library(GTest::gtest_main ALIAS gtest_main)
else()
    message(STATUS "Found system GTest installation")
endif()

# Check if ArduinoJson is available from PlatformIO
set(ARDUINO_JSON_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.pio/libdeps/native/ArduinoJson/src")

if(NOT EXISTS ${ARDUINO_JSON_DIR})
    message(STATUS "ArduinoJson not found at ${ARDUINO_JSON_DIR}")
    message(STATUS "Run 'pio run -e native' to install ArduinoJson")
    message(FATAL_ERROR "ArduinoJson dependency missing")
endif()

# Define source files
set(DSL_SOURCES
    src/automation_dsl/core.cpp
    src/automation_dsl/registry_functions.cpp
    src/automation_dsl/bridge_functions.cpp
    src/automation_dsl/time_helpers.cpp
)

set(TEST_SOURCES
    tests_native/test_mocks.cpp
    tests_native/unified_value_tests.cpp
    tests_native/function_registry_tests.cpp
    tests_native/bridge_integration_tests.cpp
)

# Create test executable
add_executable(native_tests ${DSL_SOURCES} ${TEST_SOURCES})

# Include directories
target_include_directories(native_tests PRIVATE 
    ${ARDUINO_JSON_DIR}
    src/automation_dsl
)

# Link libraries
target_link_libraries(native_tests 
    GTest::gtest_main
    pthread
)

# Enable testing
enable_testing()

# Add test cases
add_test(NAME UnifiedValueTests COMMAND native_tests --gtest_filter="UnifiedValueTest.*")
add_test(NAME FunctionRegistryTests COMMAND native_tests --gtest_filter="FunctionRegistryTest.*")  
add_test(NAME BridgeIntegrationTests COMMAND native_tests --gtest_filter="BridgeIntegrationTest.*")
add_test(NAME AllTests COMMAND native_tests)

# Custom target for running tests
add_custom_target(run_tests 
    COMMAND native_tests
    DEPENDS native_tests
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running native tests..."
)
