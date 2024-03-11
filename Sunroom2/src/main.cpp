#include <Arduino.h>
#include "definitions.h"
#include "ds18b20.h"
#include "temperatureMoisture.h"
#include "device_identity.h"
#include "servers.h"
#include "wifi_helpers.h"
#include "preferences_helpers.h"
#include "peripheral_controls.h"
#include "time_helpers.h"

// look into: https://github.com/kj831ca/KasaSmartPlug

// void serverTask(void *parameter)
// {
//   for (;;)
//   {
//     serverLoop();
//     delay(1);
//   }
// }

void setup(void)
{
  Serial.begin(BAUD);
  delay(200);
  setupPreferences();
  checkDeviceIdentityOnSetup();
  wifiSetup();
  // temperatureProbeSetup();
  peripheralControlsSetup();
  serverSetup();
  // xTaskCreatePinnedToCore(
  //     serverTask,   /* Function to implement the task */
  //     "serverTask", /* Name of the task */
  //     10000,        /* Stack size in words */
  //     NULL,         /* Task input parameter */
  //     0,            /* Priority of the task */
  //     NULL,         /* Task handle. */
  //     0             /* Core where the task should run */
  // );
  Serial.println("~~~ SETUP FINISHED ~~~");
}

void loop()
{
  wifiCheckInLoop();
  updateTimeLoop();
  temperatureMoistureLoop();
  // temperatureProbeLoop();
  controlPeripheralsLoop();
  delay(10);
}

// 132kb