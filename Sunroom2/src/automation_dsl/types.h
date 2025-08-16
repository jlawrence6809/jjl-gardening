#pragma once
#include <ArduinoJson.h>
#include <functional>

enum TypeCode
{
    ERROR_TYPE = -1,
    VOID_TYPE = 0,
    FLOAT_TYPE = 1,
    BOOL_ACTUATOR_TYPE = 2
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
    TIME_ERROR = 11,
};

struct RuleReturn
{
    TypeCode type;
    ErrorCode errorCode;
    float val;
    std::function<void(float)> actuatorSetter;
};

void processRelayRules();
