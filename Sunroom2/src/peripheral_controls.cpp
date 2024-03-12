#include <Arduino.h>
#include <time.h>
#include "definitions.h"
#include "interval_timer.h"
#include "time_helpers.h"
#include <ArduinoJson.h>
#include "rule_helpers.h"

static Timer timer(5000);
int SUNROOM_LIGHTS_RELAY = 6;

void turnOffRelay(int relay)
{
    int pin = RELAY_PINS[relay];
    digitalWrite(pin, HIGH);
    RELAY_VALUES[relay] = false;
}

void turnOnRelay(int relay)
{
    int pin = RELAY_PINS[relay];
    digitalWrite(pin, LOW);
    RELAY_VALUES[relay] = true;
}

void setupRelays()
{
    for (int i = 0; i < RELAY_COUNT; i++)
    {
        int pin = RELAY_PINS[i];
        pinMode(pin, OUTPUT);
        turnOffRelay(pin);
    }
}

void relayRefresh()
{
    for (int i = 0; i < RELAY_COUNT; i++)
    {
        digitalWrite(RELAY_PINS[i], !RELAY_VALUES[i]);
    }
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
    setupRelays();
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
    LIGHT_LEVEL = analogRead(PHOTO_SENSOR_PIN);

    Serial.println("Processing relay rules...");
    processRelayRules();
}
