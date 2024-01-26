#pragma once

void writeWifiCredentials(String ssid, String password);
void writeEnvironmentalControlValues(float temperature, float temperatureRange, float humidity, float humidityRange, bool useNaturalLightingCycle, int turnLightsOnAtMinute, int turnLightsOffAtMinute);
void setupPreferences();