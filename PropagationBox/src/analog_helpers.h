#pragma once

/**
 * Reads the ADC on the given pin and returns the voltage
 * Includes corrections for the ADC for some given devices
 */
float readADC(int pin);

/**
 * Reads the thermistor on the given pin and returns the temperature in Fahrenheit
 */
float readTFahFromADC(int pin);
float setDAC(int v);
