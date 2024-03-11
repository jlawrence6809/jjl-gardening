#include <Arduino.h>
#include <time.h>
#include "definitions.h"
#include "interval_timer.h"
#include "time_helpers.h"

static Timer timer(30000);
int SUNROOM_LIGHTS_RELAY = RELAY_7_PIN;

void turnOffRelay(int pin)
{
    digitalWrite(pin, HIGH);
    RELAY_VALUES[pin] = false;
}

void turnOnRelay(int pin)
{
    digitalWrite(pin, LOW);
    RELAY_VALUES[pin] = true;
}

void setupRelay(int pin)
{
    pinMode(pin, OUTPUT);
    turnOffRelay(pin);
}

void relayRefresh()
{
    digitalWrite(RELAY_1_PIN, !RELAY_VALUES[RELAY_1_PIN]);
    digitalWrite(RELAY_2_PIN, !RELAY_VALUES[RELAY_2_PIN]);
    digitalWrite(RELAY_3_PIN, !RELAY_VALUES[RELAY_3_PIN]);
    digitalWrite(RELAY_4_PIN, !RELAY_VALUES[RELAY_4_PIN]);
    digitalWrite(RELAY_5_PIN, !RELAY_VALUES[RELAY_5_PIN]);
    digitalWrite(RELAY_6_PIN, !RELAY_VALUES[RELAY_6_PIN]);
    digitalWrite(RELAY_7_PIN, !RELAY_VALUES[RELAY_7_PIN]);
    digitalWrite(RELAY_8_PIN, !RELAY_VALUES[RELAY_8_PIN]);
}

void photoSensorSetup()
{
    // analog for PHOTO_SENSOR_PIN
    pinMode(PHOTO_SENSOR_PIN, INPUT);

    // analog setup:
    adcAttachPin(PHOTO_SENSOR_PIN);

    // analog read:
    int value = analogRead(PHOTO_SENSOR_PIN);
}

void lightSwitchSetup()
{
    pinMode(LIGHT_SWITCH_PIN, INPUT);
    IS_SWITCH_ON = digitalRead(LIGHT_SWITCH_PIN);
}

// void turnOffFan()
// {
//     digitalWrite(FAN_PIN, LOW);
//     IS_FAN_ON = false;
// }

// void turnOnFan()
// {
//     digitalWrite(FAN_PIN, HIGH);
//     IS_FAN_ON = true;
// }

// void turnOffHeatMat()
// {
//     digitalWrite(HEAT_MAT_PIN, LOW);
//     IS_HEAT_MAT_ON = false;
// }

// void turnOnHeatMat()
// {
//     digitalWrite(HEAT_MAT_PIN, HIGH);
//     IS_HEAT_MAT_ON = true;
// }

void turnOffAllPeripherals()
{
    // turnOffFan();
    // turnOffHeatMat();
    // RELAY_VALUES[RELAY_1_PIN] = 0;
    // RELAY_VALUES[RELAY_2_PIN] = 0;
    // RELAY_VALUES[RELAY_3_PIN] = 0;
    // RELAY_VALUES[RELAY_4_PIN] = 0;
    // RELAY_VALUES[RELAY_5_PIN] = 0;
    // RELAY_VALUES[RELAY_6_PIN] = 0;
    // RELAY_VALUES[RELAY_7_PIN] = 0;
    // RELAY_VALUES[RELAY_8_PIN] = 0;
}

void peripheralControlsSetup()
{
    setupRelay(RELAY_1_PIN);
    setupRelay(RELAY_2_PIN);
    setupRelay(RELAY_3_PIN);
    setupRelay(RELAY_4_PIN);
    setupRelay(RELAY_5_PIN);
    setupRelay(RELAY_6_PIN);
    setupRelay(RELAY_7_PIN);
    setupRelay(RELAY_8_PIN);
    photoSensorSetup();
    lightSwitchSetup();
}

void lightSwitchLoop()
{
    int switchV = digitalRead(LIGHT_SWITCH_PIN);
    if (switchV != IS_SWITCH_ON)
    {
        bool sunroomV = RELAY_VALUES[SUNROOM_LIGHTS_RELAY];
        if (switchV == 1 && sunroomV == 0)
        {
            turnOnRelay(SUNROOM_LIGHTS_RELAY);
        }
        else if (switchV == 0 && sunroomV == 1)
        {
            turnOffRelay(SUNROOM_LIGHTS_RELAY);
        }
        IS_SWITCH_ON = switchV;
    }
}

/**
 * Controls the peripherals based on the current temperature and humidity
 */
void controlPeripheralsLoop()
{
    lightSwitchLoop();
    relayRefresh();
    if (!timer.isIntervalPassed())
    {
        return;
    }

    Serial.println("Free heap:");
    Serial.println(ESP.getFreeHeap());

    Serial.println("Checking peripherals...");

    LIGHT_LEVEL = analogRead(PHOTO_SENSOR_PIN);

    // 1: if the temperature is too high outside of the variance we need to turn off the heat mat
    // 2: if it is double the variance then we need to emergency turn off the lights and turn on the fan
    // 3: if the temperature is too low then we turn on the heat mat
    // 4: if the humidity is too high then we turn on the fan
    // 5: if the humidity is too low we need to turn off the fan
    // 6: if the time is not between the lights on and off time then we need to turn off the lights
    // 7: if the time is between the lights on and off time then we need to turn on the lights
    // 8: if USE_NATURAL_LIGHTING_CYCLE is true then we need to adjust the pwm of the lights to simulate the sun dimming and brightening

    // use time library to get current minute of the day:
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        Serial.println("Failed to obtain time");
        turnOffAllPeripherals();
        return;
    }

    if (
        CURRENT_TEMPERATURE == NULL_TEMPERATURE || CURRENT_HUMIDITY == NULL_TEMPERATURE || CURRENT_PROBE_TEMPERATURE == NULL_TEMPERATURE || DESIRED_TEMPERATURE == NULL_TEMPERATURE || DESIRED_HUMIDITY == NULL_TEMPERATURE)
    {
        Serial.println("Peripheral controls not initialized, skipping...");
        turnOffAllPeripherals();
        return;
    }

    // 1:
    if (CURRENT_TEMPERATURE > DESIRED_TEMPERATURE + TEMPERATURE_RANGE)
    {
        // turn off heat mat
        Serial.println("Temperature too high, turning off heat mat");
        // turnOffHeatMat();
    }

    // 2:
    if (CURRENT_TEMPERATURE > DESIRED_TEMPERATURE + TEMPERATURE_RANGE * 2)
    {
        // Emergency turn off lights and turn on fan
        Serial.println("Emergency! Temperature much too high, turning off lights and turning on fan");
        // turn off lights
        // turnOffLed();
        // turn on fan
        // turnOnFan();
        return;
    }

    // 3:
    else if (CURRENT_TEMPERATURE < DESIRED_TEMPERATURE - TEMPERATURE_RANGE)
    {
        Serial.println("Temperature too low, turning on heat mat");
        // turn on heat mat
        // turnOnHeatMat();
    }

    // 4:
    if (CURRENT_HUMIDITY > DESIRED_HUMIDITY + HUMIDITY_RANGE)
    {
        Serial.println("Humidity too high, turning on fan");
        // turn on fan
        // turnOnFan();
    }

    // 5:
    else if (CURRENT_HUMIDITY < DESIRED_HUMIDITY - HUMIDITY_RANGE)
    {
        Serial.println("Humidity too low, turning off fan");
        // turn off fan
        // turnOffFan();
    }

    // For ease of calculation, normalize the times to the light turn on time
    int normalizedStartMinute = 0;
    int normalizedStopMinute = normalizeTimeToStartTime(TURN_LIGHTS_OFF_AT_MINUTE, TURN_LIGHTS_ON_AT_MINUTE);
    int currentMinute = (timeinfo.tm_hour * 60) + timeinfo.tm_min;
    int normalizedCurrentMinute = normalizeTimeToStartTime(currentMinute, TURN_LIGHTS_ON_AT_MINUTE);

    // 6:
    if (normalizedCurrentMinute > normalizedStopMinute)
    {
        Serial.println("Time to turn off lights");
        // turn off lights
        // turnOffLed();
    }
    else if (USE_NATURAL_LIGHTING_CYCLE)
    {
        Serial.println("Time to turn on lights, using natural lighting cycle");
        // 8:
        // Calculate the current minute as a fraction of the total light cycle duration
        float fractionOfDay = normalizedCurrentMinute / (float)normalizedStopMinute;

        // Calculate the intensity using a sine wave that goes from 0 to 1 to 0
        float intensity = sin(PI * fractionOfDay);

        // Scale the intensity from 20% to 100%
        intensity = 0.2 + 0.8 * intensity;

        // setLedLevel(intensity);
    }
    else
    {
        Serial.println("Time to turn on lights");
        // 7:
        // turnOnLed();
    }
}
