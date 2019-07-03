#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include "DHTesp.h"

#ifndef STASSID
#define STASSID "Jeremy"
#define STAPSK  "3240winnie"
#endif

const char* ssid = STASSID;
const char* password = STAPSK;

unsigned long check_wifi = 30000;

ESP8266WebServer server(80);
DHTesp dht;

void handleRoot() {
  float humidity = dht.getHumidity();
  float temperature = dht.getTemperature();
  String body = "{";
  body += "t: " + String(dht.toFahrenheit(temperature), 0);
  body += ", h: " + String(humidity, 0);
  body += " }";
  Serial.println(body);
  server.send(200, "application/json", body);
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void setup(void) {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Not Connected");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");

  dht.setup(16, DHTesp::DHT11);
  Serial.println("DHT started");
}

void loop(void) {
  // if wifi is down, try reconnecting every 30 seconds
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected. Status: " + String(WiFi.status()));
    /*
      WL_NO_SHIELD        = 255,   // for compatibility with WiFi Shield library
      WL_IDLE_STATUS      = 0,
      WL_NO_SSID_AVAIL    = 1,
      WL_SCAN_COMPLETED   = 2,
      WL_CONNECTED        = 3,
      WL_CONNECT_FAILED   = 4,
      WL_CONNECTION_LOST  = 5,
      WL_DISCONNECTED     = 6
     */
    if(millis() > check_wifi) {
      Serial.println("Reconnecting to WiFi...");
      WiFi.disconnect();
      WiFi.begin(ssid, password);
      check_wifi = millis() + 30000;
    }
    delay(5000);
  } else {
    server.handleClient();
    MDNS.update();
  }
}
