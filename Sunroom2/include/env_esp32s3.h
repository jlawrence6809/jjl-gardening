// Per-environment configuration: esp32-s3-devkit
#pragma once

#define PIN_PROFILE_ESP32_S3_DEVKIT

#define CFG_WIFI_NAME "esp32s3"
#define CFG_DS18B20_PIN 38
#define CFG_PHOTO_SENSOR_PIN -1
#define CFG_LIGHT_SWITCH_PIN -1

#define CFG_RELAY_PINS 4,5,6,7,16,17,18,8
#define CFG_STRAPPING_PINS 32,33,25,26,27,28,29,30
#define CFG_RELAY_IS_INVERTED true,true,true,true,true,true,true,true

