// Per-environment configuration: barn (NodeMCU-32S)
#pragma once

#define PIN_PROFILE_NODEMCU32S

#define CFG_WIFI_NAME "barn"
#define CFG_DS18B20_PIN 23
#define CFG_PHOTO_SENSOR_PIN 36
#define CFG_LIGHT_SWITCH_PIN 39

#define CFG_RELAY_PINS 15,2,4,16,17,5,18,19,32,33,25,26,27
#define CFG_STRAPPING_PINS 32,33,25,26,27,28,29,30
#define CFG_RELAY_IS_INVERTED true,true,true,true,true,true,true,true,false,false,false,false,false

