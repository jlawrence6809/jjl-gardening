

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266mDNS.h>
//#include <DS18B20.h>
//#include <OneWire.h>
#include "DHTesp.h"

#define TIME_TO_SLEEP  10        /* Time ESP32 will go to sleep (in minutes) */
#define WIFI_WAITS 120 // 0.5 seconds between each

// generic esp8266
// if uploading isn't working set Upload Speed lower

const char* ssid = "Jeremy";
const char* password = "3240winnie";
const char* host = "http://192.168.29.188:9615/";

// DS18B20 ds(2); // use for one wire temp sensor: https://github.com/matmunk/DS18B20
DHTesp dht;

void setup(void) {
  Serial.begin(74880);
  pinMode(0, OUTPUT);
  digitalWrite(0, HIGH);
  delay(200);

// one wire temp sensor
//  while (ds.selectNext()) {
//    Serial.println(ds.getTempC());
//  }
  
  dht.setup(2, DHTesp::DHT11);
  TempAndHumidity tempAndHumidity = dht.getTempAndHumidity();

  digitalWrite(0, LOW);

  String body = "{";
  body += "id: " + String(ESP.getChipId());
  body += ", th: " + String(dht.toFahrenheit(tempAndHumidity.temperature), 0);
  body += ", h: " + String(tempAndHumidity.humidity, 0);
  body += " }";
  Serial.println(body);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection

  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && ++retries < WIFI_WAITS) {
    delay(500);
    Serial.print(".");
  }

  if(retries < WIFI_WAITS) {
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    
    HTTPClient http;   
    
    http.begin(host);  //Specify destination for HTTP request
    http.addHeader("Content-Type", "text/plain");             //Specify content-type header
    
    int httpResponseCode = http.POST(body);   //Send the actual POST request
    
    if(httpResponseCode>0){
      String response = http.getString();                       //Get the response to the request
      Serial.println("response:");
      Serial.println(httpResponseCode);   //Print return code
      Serial.println(response);           //Print request answer
    }else{
      Serial.print("Error on sending POST: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  } else {
    Serial.println("Failed to connect to wifi");
  }
  Serial.println("sleeping");
  delay(200);
  ESP.deepSleep(TIME_TO_SLEEP * 60e6);
}

void loop() {
}
