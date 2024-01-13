// generic esp8266
// if uploading isn't working set Upload Speed lower

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266mDNS.h>
#include "DHTesp.h"
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define WIFI_WAITS 120 // 0.5 seconds between each
#define MINIMUM_FAN_DUTY_CYCLE 5
#define REPORT_DUTY_CYCLE 7
#define TOO_HOT 80
#define TOO_HUMID 80

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)

#define FAN_GPIO 16

const char* ssid = "Jeremy";
const char* password = "3240winnie";
const char* host = "http://192.168.29.188:9615/";

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

int count = 0;
bool fanOn = false;

void setup(void) {
  Serial.begin(74880);
  setupDisplay();
}

void loop() {
  TempAndHumidity th = getTAndH();
  bool newFanOn = count % MINIMUM_FAN_DUTY_CYCLE == 0 || isTooHotOrHumid(th);
  setFan(newFanOn);
  printAndReport(th, newFanOn);  
  count++;
  Serial.println("sleeping");
  delay( 1 * 1000);
}

void printAndReport(TempAndHumidity th, bool newFanOn) {
  String body = String("{")
    + "id: " + String(ESP.getChipId())
    + ", t: " + String(dht.toFahrenheit(tempAndHumidity.temperature), 0)
    + ", h: " + String(th.humidity, 0)
    + ", fan: " + (newFanOn ? "1" : "0")
    + ", cnt: " + String(count)
    + " }";
  Serial.println(body);
  display.clearDisplay();
  display.setCursor(0,0);
  display.println(body);
  display.display();
  
  if(fanOn != newFanOn || count % REPORT_DUTY_CYCLE == 0) {
    // we don't need to send a status report too much, just when it changes should be fine
    send(body);
    fanOn = newFanOn;
  }
}

void send(String body) {
  if(WiFi.status() == WL_CONNECTED || startWifi()){
      HTTPClient http;   
      
      http.begin(host);  //Specify destination for HTTP request
      http.addHeader("Content-Type", "text/plain"); //Specify content-type header
      
      int httpResponseCode = http.POST(body); //Send the actual POST request
      
      if(httpResponseCode>0){
        String response = http.getString(); //Get the response to the request
        Serial.println("response:");
        Serial.println(httpResponseCode); //Print return code
        Serial.println(response); //Print request answer
      } else {
        Serial.print("Error on sending POST: ");
        Serial.println(httpResponseCode);
      }
      http.end(); 
  } else {
      Serial.println("Failed to connect to wifi");
  }
}

void setupDisplay() {
  display.begin(SSD1306_SWITCHCAPVCC, 0x3c);
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println(F("Welcome!"));
  display.display();
  display.setTextSize(1);
  delay(200);
}

bool startWifi() {
    Serial.println("starting wifi");
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
  
    // Wait for connection
    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && ++retries < WIFI_WAITS) {
      delay(500);
      Serial.print(".");
    }
    
    if(WiFi.status() == WL_CONNECTED) {
      Serial.println("");
      Serial.print("Connected to ");
      Serial.println(ssid);
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
      return true;
    }
    return false;
}

TempAndHumidity getTAndH() {
  DHTesp dht;
  dht.setup(2, DHTesp::DHT11);
  TempAndHumidity tempAndHumidity = dht.getTempAndHumidity();

  int retries = 0;
  while(isNaN(tempAndHumidity.temperature) && retries++ < 10) {
    delay(500);
    tempAndHumidity = dht.getTempAndHumidity();
  }
  return tempAndHumidity;
}

bool isTooHotOrHumid(tempAndHumidity) {
  float t = dht.toFahrenheit(tempAndHumidity.temperature);
  float h = tempAndHumidity.humidity;
  return isNaN(t)
  || isNaN(h)
  || t > TOO_HOT
  || h > TOO_HUMID;
}

void setFan(val) {
  pinMode(FAN_GPIO, OUTPUT);
  if(val) {
    digitalWrite(FAN_GPIO, HIGH);
  } else {
    digitalWrite(FAN_GPIO, LOW);
  }
}

bool isNaN(int v){
  return v != v;
}

bool isNaN(float v){
   return v != v;
}

