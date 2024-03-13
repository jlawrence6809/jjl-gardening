#pragma once
#include <ArduinoJson.h>
#include <functional>

enum TypeCode
{
    ERROR_TYPE = -1,
    BOOL_TYPE = 0,
    INT_TYPE = 1,
    FLOAT_TYPE = 2,
    VOID_TYPE = 3,
    TIME_TYPE = 4,
    BOOL_ACTUATOR_TYPE = 5
};

enum ErrorCode
{
    NO_ERROR = 0,
    UNREC_TYPE_ERROR = 1,
    IF_CONDITION_ERROR = 2,
    BOOL_ACTUATOR_ERROR = 3,
    AND_OR_ERROR = 4,
    NOT_ERROR = 5,
    COMPARISON_TYPE_ERROR = 6,
    COMPARISON_TYPE_EQUALITY_ERROR = 7,
    UNREC_FUNC_ERROR = 8,
    UNREC_STR_ERROR = 9,
    UNREC_ACTUATOR_ERROR = 10,
};

struct RuleReturn
{
    TypeCode type;
    ErrorCode errorCode;
    bool boolV;
    int intV;
    float floatV;
    int timeV;
    std::function<void(int)> actuatorSetter;
};

void processRelayRules();
