#include <Arduino.h>
#include "definitions.h"

/**
 * Reads the ADC on the given pin and returns the voltage
 * Includes corrections for the ADC for some given devices
 */
float readADC(int pin) {
    int sum = 0;
    for (int i = 0; i < 10; i++) {
        sum += analogRead(pin);
    }
    float v = sum / 10.0;
    v = (v / 4095) * 3.3;

    // corrections
    if (sum == (4095 * 10)) {
        return 99;
    } else if (sum == 0) {
        return -1.0;
    } else if (v < 2.52) {
        if (CHIP_ID == DEVICE_1) {
            return 1.01 * v + 0.154;
        } else if (CHIP_ID == DEVICE_2) {
            return 1.01 * v + 0.13;
        } else if (CHIP_ID == DEVICE_3) {
            return 1.01 * v + 0.136;
        } else {
            Serial.println("DEVICE NOT RECOGNIZED");
        }
    } else {
        return -1 + 2.09 * v - 0.251 * v * v;
    }
    return -1;
}

/**
 * Reads the thermistor on the given pin and returns the temperature in Fahrenheit
 */
float readTFahFromADC(int pin) {
    float v = readADC(pin);
    float r = 10000.0 * ((3.3 / v) - 1.0);
    // 3950 = beta, 298.15 = room temp
    float tKelvin = (3950 * 298.15) / (3950 + (298.15 * log(r / 10000.0)));
    float tFah = (tKelvin - 273.15) * (9.0 / 5.0) + 32;
    return tFah;
}

float setDAC(int v) {
    // todo: there is no DAC on the ESP32-C3
    // dacWrite(25, v);
    float out = 3.3 * (v / 255.0);
    // corrections
    if (CHIP_ID == DEVICE_1) {
        return 0.932 * out + 0.0842;
    } else if (CHIP_ID == DEVICE_2) {
        return 0.949 * out + 0.0963;
    } else if (CHIP_ID == DEVICE_3) {
        return 0.923 * out + 0.123;
    }
    Serial.println("DEVICE NOT RECOGNIZED");
    return -1;
}