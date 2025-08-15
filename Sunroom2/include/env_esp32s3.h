// Per-environment configuration: esp32-s3-devkit
#pragma once
// http://wiki.fluidnc.com/en/hardware/ESP32-S3_Pin_Reference

#define PIN_PROFILE_ESP32_S3_DEVKIT

#define CFG_WIFI_NAME "barn"
#define CFG_DS18B20_PIN 38
#define CFG_PHOTO_SENSOR_PIN -1
#define CFG_LIGHT_SWITCH_PIN -1

#define CFG_RELAY_PINS 4,5,6,7,16,17,18,8
// NOTE: Pin 4 has weak pullup on reset.
#define CFG_VALID_GPIO_PINS 1,2,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,21,38,39,40,41,42,47,48
#define CFG_RESERVED_PINS 0,3,19,20,35,36,37,43,44,45,46
#define CFG_RELAY_IS_INVERTED true,true,true,true,true,true,true,true

#define CFG_I2C_SCL_PIN 9
#define CFG_I2C_SDA_PIN 8