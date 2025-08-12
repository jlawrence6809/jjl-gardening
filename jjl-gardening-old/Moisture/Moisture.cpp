// #include <DHTesp.h>
#include <DS18B20.h>
#include <Arduino.h>
#include "credentials.h"
#include "definitions.h"

// VARIABLES
const char *APP_NAME = "PropagationBox";

uint64_t CHIP_ID = 0;
uint64_t DEVICE_1 = 0xa85627a4ae30;
uint64_t DEVICE_2 = 0xc8216c12cfa4;
uint64_t DEVICE_3 = 0xe0286c12cfa4;

int DHT_PIN = 17;

const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;

// DHT stuff
// DHTesp dht;
// TaskHandle_t tempTaskHandle = NULL;

void setup(void)
{
  Serial.begin(BAUD);
  delay(200);
  checkDeviceIdentityOnSetup();
  wifiSetup();
  serverSetup();
  Serial.println("HTTP server started");
  // dht.setup(DHT_PIN, DHTesp::DHT11);
  // Serial.println("DHT setup");
}

void loop()
{
  bool wifiIsConnected = wifiCheckInLoop();
  if (wifiIsConnected)
  {
    serverLoop();
  }
}
