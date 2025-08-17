#include <cmath>
#include <functional>
#include <iostream>
#include <vector>

#include <gtest/gtest.h>
#include "../src/automation_dsl/unified_value.h"

// Test fixture class for shared setup
class UnifiedValueTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// Test 1: Basic construction and type checking
TEST_F(UnifiedValueTest, BasicConstructionAndTypeChecking) {
    UnifiedValue floatVal(25.5f);
    UnifiedValue intVal(42);
    UnifiedValue stringVal("connected");
    UnifiedValue voidVal = UnifiedValue::createVoid();
    UnifiedValue errorVal = UnifiedValue::createError(PARSE_ERROR);

    EXPECT_EQ(floatVal.type, UnifiedValue::FLOAT_TYPE);
    EXPECT_EQ(intVal.type, UnifiedValue::INT_TYPE);
    EXPECT_EQ(stringVal.type, UnifiedValue::STRING_TYPE);
    EXPECT_EQ(voidVal.type, UnifiedValue::VOID_TYPE);
    EXPECT_EQ(errorVal.type, UnifiedValue::ERROR_TYPE);

    EXPECT_EQ(floatVal.errorCode, NO_ERROR);
    EXPECT_EQ(intVal.errorCode, NO_ERROR);
    EXPECT_EQ(stringVal.errorCode, NO_ERROR);
    EXPECT_EQ(voidVal.errorCode, NO_ERROR);
    EXPECT_EQ(errorVal.errorCode, PARSE_ERROR);
}

// Test 2: Type conversions
TEST_F(UnifiedValueTest, TypeConversions) {
    UnifiedValue floatVal(25.5f);
    UnifiedValue intVal(42);
    UnifiedValue stringFloatVal("123.45");
    UnifiedValue stringIntVal("456");
    UnifiedValue badStringVal("not_a_number");

    // Float conversions
    EXPECT_FLOAT_EQ(floatVal.asFloat(), 25.5f);
    EXPECT_EQ(floatVal.asInt(), 25);

    // Int conversions
    EXPECT_FLOAT_EQ(intVal.asFloat(), 42.0f);
    EXPECT_EQ(intVal.asInt(), 42);

    // String conversions
    EXPECT_FLOAT_EQ(stringFloatVal.asFloat(), 123.45f);
    EXPECT_EQ(stringIntVal.asInt(), 456);
    EXPECT_FLOAT_EQ(badStringVal.asFloat(), 0.0f);
    EXPECT_EQ(badStringVal.asInt(), 0);

    // Two-stage int parsing
    EXPECT_EQ(stringFloatVal.asInt(), 123);

    // String representations
    EXPECT_STREQ(floatVal.asString(), "25.500");
    EXPECT_STREQ(intVal.asString(), "42");
    EXPECT_STREQ(stringFloatVal.asString(), "123.45");
}

// Test 3: Error type conversions
TEST_F(UnifiedValueTest, ErrorTypeConversions) {
    UnifiedValue errorVal = UnifiedValue::createError(TIME_ERROR);
    UnifiedValue voidVal = UnifiedValue::createVoid();

    EXPECT_FLOAT_EQ(errorVal.asFloat(), 0.0f);
    EXPECT_EQ(errorVal.asInt(), 0);
    EXPECT_STREQ(errorVal.asString(), "time_error");

    EXPECT_FLOAT_EQ(voidVal.asFloat(), 0.0f);
    EXPECT_EQ(voidVal.asInt(), 0);
    EXPECT_STREQ(voidVal.asString(), "void");
}

// Test 4: Actuator functions
TEST_F(UnifiedValueTest, ActuatorFunctions) {
    float capturedValue = -1.0f;
    auto setterFunction = [&capturedValue](float value) { capturedValue = value; };

    UnifiedValue actuatorVal = UnifiedValue::createActuator(setterFunction);

    EXPECT_EQ(actuatorVal.type, UnifiedValue::ACTUATOR_TYPE);
    EXPECT_EQ(actuatorVal.errorCode, NO_ERROR);
    EXPECT_STREQ(actuatorVal.asString(), "actuator");

    // Test actuator function execution
    auto retrievedSetter = actuatorVal.getActuatorSetter();
    EXPECT_NE(retrievedSetter, nullptr);

    if (retrievedSetter) {
        retrievedSetter(42.5f);
        EXPECT_FLOAT_EQ(capturedValue, 42.5f);
    }

    // Test non-actuator getActuatorSetter
    UnifiedValue floatVal(25.0f);
    auto nullSetter = floatVal.getActuatorSetter();
    EXPECT_EQ(nullSetter, nullptr);
}

// Test 5: Comparison operators
TEST_F(UnifiedValueTest, ComparisonOperators) {
    UnifiedValue float1(25.5f);
    UnifiedValue float2(25.5f);
    UnifiedValue float3(30.0f);
    UnifiedValue int1(25);
    UnifiedValue string1("connected");
    UnifiedValue string2("connected");
    UnifiedValue string3("disconnected");
    UnifiedValue error1 = UnifiedValue::createError(PARSE_ERROR);
    UnifiedValue error2 = UnifiedValue::createError(TIME_ERROR);

    // Float equality
    EXPECT_TRUE(float1 == float2);
    EXPECT_FALSE(float1 == float3);

    // String equality
    EXPECT_TRUE(string1 == string2);
    EXPECT_FALSE(string1 == string3);

    // Cross-type comparison (as floats)
    EXPECT_FALSE(float1 == int1);  // 25.5 != 25 (float vs int)

    // Error comparisons (always false)
    EXPECT_FALSE(error1 == error2);
    EXPECT_FALSE(error1 == float1);

    // Ordering comparisons
    EXPECT_TRUE(float1 < float3);   // 25.5 < 30.0
    EXPECT_TRUE(float3 > float1);   // 30.0 > 25.5
    EXPECT_TRUE(float1 <= float2);  // 25.5 <= 25.5
    EXPECT_TRUE(float1 >= float2);  // 25.5 >= 25.5

    // Error ordering (always false)
    EXPECT_FALSE(error1 < float1);
    EXPECT_FALSE(error1 > float1);
}

// Test 6: Copy constructor and assignment
TEST_F(UnifiedValueTest, CopyConstructorAndAssignment) {
    UnifiedValue original(42.5f);
    UnifiedValue copy1(original);
    UnifiedValue copy2 = original;

    EXPECT_EQ(copy1.type, UnifiedValue::FLOAT_TYPE);
    EXPECT_FLOAT_EQ(copy1.asFloat(), 42.5f);
    EXPECT_EQ(copy1.errorCode, NO_ERROR);

    EXPECT_EQ(copy2.type, UnifiedValue::FLOAT_TYPE);
    EXPECT_FLOAT_EQ(copy2.asFloat(), 42.5f);
    EXPECT_EQ(copy2.errorCode, NO_ERROR);

    // Test string copying
    UnifiedValue stringOriginal("test string");
    UnifiedValue stringCopy(stringOriginal);
    EXPECT_STREQ(stringCopy.asString(), "test string");

    // Test actuator copying
    float captured = -1.0f;
    auto setter = [&captured](float v) { captured = v; };
    UnifiedValue actuatorOriginal = UnifiedValue::createActuator(setter);
    UnifiedValue actuatorCopy(actuatorOriginal);

    EXPECT_EQ(actuatorCopy.type, UnifiedValue::ACTUATOR_TYPE);
    auto copiedSetter = actuatorCopy.getActuatorSetter();
    if (copiedSetter) {
        copiedSetter(123.0f);
        EXPECT_FLOAT_EQ(captured, 123.0f);
    }
}

// Test 7: Strict string parsing
TEST_F(UnifiedValueTest, StrictStringParsing) {
    // Valid conversions
    UnifiedValue validFloat("123.45");
    UnifiedValue validInt("456");
    UnifiedValue validZero("0");
    UnifiedValue validZeroFloat("0.0");

    EXPECT_FLOAT_EQ(validFloat.asFloat(), 123.45f);
    EXPECT_EQ(validInt.asInt(), 456);
    EXPECT_EQ(validZero.asInt(), 0);
    EXPECT_FLOAT_EQ(validZeroFloat.asFloat(), 0.0f);

    // Invalid conversions (should return 0)
    UnifiedValue invalidFloat("123.45abc");
    UnifiedValue invalidInt("123abc");
    UnifiedValue whitespace("123.45 ");
    UnifiedValue empty("");
    UnifiedValue letters("abc");

    EXPECT_FLOAT_EQ(invalidFloat.asFloat(), 0.0f);
    EXPECT_EQ(invalidInt.asInt(), 0);
    EXPECT_FLOAT_EQ(whitespace.asFloat(), 0.0f);
    EXPECT_FLOAT_EQ(empty.asFloat(), 0.0f);
    EXPECT_FLOAT_EQ(letters.asFloat(), 0.0f);

    // Two-stage int parsing consistency
    UnifiedValue directFloat(25.7f);
    UnifiedValue stringFloat("25.7");
    UnifiedValue negativeFloat("-3.8");

    EXPECT_EQ(directFloat.asInt(), stringFloat.asInt());
    EXPECT_EQ(stringFloat.asInt(), 25);
    EXPECT_EQ(negativeFloat.asInt(), -3);
}

// Test 8: Factory methods
TEST_F(UnifiedValueTest, FactoryMethods) {
    auto voidResult = UnifiedValue::createVoid();
    auto errorResult = UnifiedValue::createError(UNREC_FUNC_ERROR);
    auto actuatorResult = UnifiedValue::createActuator([](float) {});

    EXPECT_EQ(voidResult.type, UnifiedValue::VOID_TYPE);
    EXPECT_EQ(voidResult.errorCode, NO_ERROR);

    EXPECT_EQ(errorResult.type, UnifiedValue::ERROR_TYPE);
    EXPECT_EQ(errorResult.errorCode, UNREC_FUNC_ERROR);

    EXPECT_EQ(actuatorResult.type, UnifiedValue::ACTUATOR_TYPE);
    EXPECT_EQ(actuatorResult.errorCode, NO_ERROR);
}

// Test 9: Error code string conversion
TEST_F(UnifiedValueTest, ErrorCodeStringConversion) {
    auto parseError = UnifiedValue::createError(PARSE_ERROR);
    auto timeError = UnifiedValue::createError(TIME_ERROR);
    auto unknownError = UnifiedValue::createError(static_cast<ErrorCode>(999));

    EXPECT_STREQ(parseError.asString(), "parse_error");
    EXPECT_STREQ(timeError.asString(), "time_error");
    EXPECT_STREQ(unknownError.asString(), "unknown_error");
}

// Test 10: Memory management and edge cases
TEST_F(UnifiedValueTest, MemoryManagementAndEdgeCases) {
    // Test null string handling
    UnifiedValue nullString(static_cast<const char*>(nullptr));
    EXPECT_STREQ(nullString.asString(), "");

    // Test self-assignment
    UnifiedValue value(42.0f);
    value = value;
    EXPECT_FLOAT_EQ(value.asFloat(), 42.0f);

    // Test complex type reassignment
    UnifiedValue value1("test string");
    UnifiedValue value2(123.45f);
    value1 = value2; // Should properly destroy string and copy float
    EXPECT_EQ(value1.type, UnifiedValue::FLOAT_TYPE);
    EXPECT_FLOAT_EQ(value1.asFloat(), 123.45f);
}