#include <Arduino.h>
#include "credentials.h"
#include "definitions.h"
#include "ds18b20.h"
#include "temperatureMoisture.h"
#include "device_identity.h"
#include "servers.h"
#include "wifi_helpers.h"
#include "preferences_helpers.h"
#include "peripheral_controls.h"
#include "time_helpers.h"
#include "pwm_led.h"

void setup(void)
{
  Serial.begin(BAUD);
  delay(200);
  setupPreferences();
  checkDeviceIdentityOnSetup();
  wifiSetup();
  serverSetup();
  pwmLedSetup();
  temperatureProbeSetup();
  peripheralControlsSetup();
  Serial.println("~~~ SETUP FINISHED ~~~");
}

void loop()
{
  wifiCheckInLoop();
  serverLoop();
  updateTimeLoop();
  temperatureMoistureLoop();
  temperatureProbeLoop();
  controlPeripheralsLoop();
  delay(100);
}
