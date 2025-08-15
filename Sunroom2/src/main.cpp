#ifndef UNIT_TEST
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
#include "system_status.h"

// Keep an eye on this: https://github.com/microsoft/devicescript

// look into: https://github.com/kj831ca/KasaSmartPlug

void setup(void)
{
  // 10 secon delay to prevent boot loop from wrecking the flash memory
  delay(10000);
  Serial.begin(BAUD);
  setupPreferences();
  checkDeviceIdentityOnSetup();
  wifiSetup();
  // temperatureProbeSetup();
  peripheralControlsSetup();
#ifndef UNIT_TEST
  serverSetup();
#endif
  Serial.println("~~~ SETUP FINISHED ~~~");
}

void loop()
{
  wifiCheckInLoop();
  updateTimeLoop();
  temperatureMoistureLoop();
  // temperatureProbeLoop();
  controlPeripheralsLoop();
  systemStatusLoop();
  // delay(500);
  // Serial.println("~~~ LOOP FINISHED ~~~");
  delay(1);
}
#endif // UNIT_TEST
