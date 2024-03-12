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

// Keep an eye on this: https://github.com/microsoft/devicescript

// look into: https://github.com/kj831ca/KasaSmartPlug

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
