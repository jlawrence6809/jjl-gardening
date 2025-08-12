// generic esp8266
// if uploading isn't working set Upload Speed lower
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266mDNS.h>
// #include "DHTesp.h"
#include <SPI.h>
#include <Wire.h>
// #include <Adafruit_GFX.h>
// #include <Adafruit_SSD1306.h>
#include "credentials.h"

#define WIFI_WAITS 120 // 0.5 seconds between each
#define MINIMUM_FAN_DUTY_CYCLE 5
#define REPORT_DUTY_CYCLE 7

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET    -1 // Reset pin # (or -1 if sharing Arduino reset pin)

#define FAN_GPIO 16
#define HEATER_GPIO 14
#define LIGHT_GPIO 12
#define WATER_GPIO 13
#define DHT_GPIO 15
#define BUTTON_1 0
#define BUTTON_2 2

#define MIN_DELAY_MS 500
#define DEBOUNCE_PERIOD 250

const int debounceCycles = DEBOUNCE_PERIOD / MIN_DELAY_MS;
int button1DebounceCount = 0;
int button2DebounceCount = 0;

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;
const char* host = "http://192.168.29.188:9615/";

// Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
// DHTesp dht;

int uptime_ms = 0; // todo

int tempLimit = 80;
int humLimit = 80;
bool fanOn = false;
bool heaterOn = false;
bool lightOn = false;

#define status_state 0
//#define 24Stats_state 1
#define opts1_state 2
#define opts2_state 3
#define opts3_state 4
#define opts4_state 5
#define targets_state 6
#define override_state 7
#define wifi_state 8
#define sysinfo_state 9

// class DisplayStateType {
//   public:
//   virtual void Display(Adafruit_SSD1306 display);
//   virtual DisplayStateType Button1Action();
//   virtual DisplayStateType Button2Action();
// };

// class StatusState : DisplayStateType {
//   public:
//   static StatusState getInstance(){
//     static StatusState inst;
//     return inst;
//   }
//   public:
//   void Display(Adafruit_SSD1306 display){
//     // todo display state  
//   }
//   DisplayStateType Button1Action() {
//     return 24StatsState.getInstance();
//   }
//   DisplayStateType Button2Action(){
//     return this; // todo
//   }
// };

// class 24StatsState : DisplayStateType {
//   public:
//   static 24StatsState getInstance(){
//     static 24StatsState inst;
//     return inst;
//   }
//   public:
//   void Display(Adafruit_SSD1306 display){
//     // todo display state  
//   }
//   DisplayStateType Button1Action() {
//     return this; // todo
//   }
//   DisplayStateType Button2Action(){
//     return this; // todo
//   }
// };

// DisplayStateType displayState = StatusState.getInstance();


void setup(void) {
  // setup state map
  Serial.begin(74880);
  // print "starting" to Serial
  Serial.println("starting");

  startWifi();

  // display.begin(SSD1306_SWITCHCAPVCC, 0x3c);
  // display.clearDisplay();
  // display.setTextSize(2);
  // display.setTextColor(WHITE);
  // display.setCursor(0,0);
  // display.println(F("Welcome!"));
  // display.display();
  // display.setTextSize(1);
  delay(200);
}

void loop() {
  // if button 1
  // if button 2
  // if sense and report timeout
  // keep track of time

//   if(button1DebounceCount > 0) {
//     button1DebounceCount--;
//   } else if(digitalRead(BUTTON_1)){
// //    button1Logic();
//     button1DebounceCount = debounceCycles;
//   }
//   if(button2DebounceCount > 0) {
//     button2DebounceCount--;
//   } else if(digitalRead(BUTTON_2)){
// //    button2Logic();
//     button2DebounceCount = debounceCycles;
//   }

  // if button hasn't been triggered in x ms  
  uptime_ms += MIN_DELAY_MS;
  delay(MIN_DELAY_MS);
  Serial.println("LOOP");
}

/*

{
  id: 'd_status',
  button1Action: () => displayState = d_24Stats,
  button2Action: () => displayState = d_Opts1,
  display: () => {},
}
*/

void senseAndReport() {
    // get humidity
  // TempAndHumidity tempAndHumidity = getDHT();
  // float t = dht.toFahrenheit(tempAndHumidity.temperature);
  // float h = tempAndHumidity.humidity;
  float t = 70;
  float h = 70;

  bool tooHotOrHumid = 
    isNaN(t)
    || isNaN(h)
    || t > tempLimit
    || h > humLimit;

  bool newFanOn = tooHotOrHumid || uptime_ms % MINIMUM_FAN_DUTY_CYCLE == 0;
  
  pinMode(FAN_GPIO, OUTPUT);
  digitalWrite(FAN_GPIO, newFanOn ? HIGH : LOW);
  pinMode(LIGHT_GPIO, OUTPUT);
  digitalWrite(LIGHT_GPIO, tooHotOrHumid ? LOW : HIGH);
  pinMode(HEATER_GPIO, OUTPUT);
  digitalWrite(HEATER_GPIO, tooHotOrHumid ? LOW : HIGH);

  bool hasAnythingChanged = newFanOn != fanOn || tooHotOrHumid == lightOn || tooHotOrHumid == heaterOn;

  fanOn = newFanOn;
  lightOn = !tooHotOrHumid;
  heaterOn = !tooHotOrHumid;

//  display.clearDisplay();
//  display.setCursor(0,0);
//  display.println(body);
//  display.display();

  if(hasAnythingChanged || uptime_ms % REPORT_DUTY_CYCLE == 0) {
    // we don't need to send a status report too much, just when it changes should be fine
    String body = String("{")
      + "id: " + String(ESP.getChipId())
      + ", t: " + String(t, 0)
      + ", h: " + String(h, 0)
      + ", fan: " + (fanOn ? "1" : "0")
      + ", light: " + (lightOn ? "1" : "0")
      + ", heat: " + (heaterOn ? "1" : "0")
      + " }";
    send(body);
  }
}

// TempAndHumidity getDHT() {
//   dht.setup(DHT_GPIO, DHTesp::DHT11);
//   TempAndHumidity tempAndHumidity = dht.getTempAndHumidity();

//   int retries = 0;
//   while(isNaN(tempAndHumidity.temperature) && retries++ < 10) {
//     delay(500);
//     tempAndHumidity = dht.getTempAndHumidity();
//   }
//   return tempAndHumidity;
// }

void send(String body) {
  if(WiFi.status() == WL_CONNECTED || startWifi()){
      HTTPClient http;
      WiFiClient client; // Create a WiFiClient object
      
      http.begin(client, host);  // Pass the client and the URL
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

void setupWifi(){
  // button was clicked in options
  // change to wifi station
  // indicate on screen what to connect to
  // serve up webpage allowing user to input ssid and pass
  // on input display submitted message on website
  // save input to eeprom: https://circuits4you.com/2016/12/16/esp8266-internal-eeprom-arduino/
  // display saved message on display
  // switch back to wifi station mode
  // attempt to connect
  // display "connecting" to user
  // on success display success message
  // on failure allow user to try to connect again, to re-enter wifi info, and to exit
}

void displayStatus(){
  
}

void display24HrStats(){
  
}

void displayOptions(){
  
}

bool isNaN(int v){
  return v != v;
}

bool isNaN(float v){
   return v != v;
}



