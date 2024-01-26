#include <Arduino.h>
#include "definitions.h"

// setting PWM properties
constexpr int PWM_FREQ = 5000;
constexpr int PWM_CHANNEL = 0;
constexpr int PWM_RESOLUTION = 8;

void pwmLedSetup()
{
    // configure LED PWM functionalitites
    ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);

    // attach the channel to the GPIO to be controlled
    ledcAttachPin(LED_PIN, PWM_CHANNEL);
}

void turnOnLed()
{
    ledcWrite(PWM_CHANNEL, 255);
    LED_LEVEL = 1;
}

void turnOffLed()
{
    ledcWrite(PWM_CHANNEL, 0);
    LED_LEVEL = 0;
}

void setLedLevel(float level)
{
    ledcWrite(PWM_CHANNEL, level * 255);
    LED_LEVEL = level;
}
