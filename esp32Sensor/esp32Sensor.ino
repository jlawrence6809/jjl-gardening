#include <WiFi.h>
#include <HTTPClient.h>
#include <ESPmDNS.h>
#include <DHTesp.h>
#include <Update.h>
#include <soc/efuse_reg.h>

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  10        /* Time ESP32 will go to sleep (in minutes) */
#define WIFI_WAITS 120 // 0.5 seconds between each

// board "node32s"
// e fuse isn't written! (adc vref value)
uint64_t chipid;
#define DEVICE_1 0xa85627a4ae30
#define DEVICE_2 0xc8216c12cfa4
#define DEVICE_3 0xe0286c12cfa4

const char* ssid = "Jeremy";
const char* password = "3240winnie";
const char* host = "http://192.168.29.188:9615/";

// DHT stuff
DHTesp dht;
TaskHandle_t tempTaskHandle = NULL;
int dhtPin = 17;

// Thermistor stuff
#define THERM_SERIES 9900
#define THERM_NOM 10000

float readADC(int pin){
  int sum = 0;
  for(int i = 0; i < 10; i++){
    sum += analogRead(pin);
  }
  float v = sum / 10.0;
  if(true){
    return v;
  }
  v = (v / 4095) * 3.3;

  //corrections
  if(sum == (4095*10)) {
    return 99;
  } else if(sum == 0) {
    return -1.0;
  } else if(v < 2.52) {
    if(chipid == DEVICE_1){
      return 1.01*v + 0.154;
    } else if(chipid == DEVICE_2){
      return 1.01*v + 0.13;
    } else if(chipid == DEVICE_3){
      return 1.01*v + 0.136;
    } else {
      Serial.println("DEVICE NOT RECOGNIZED");
    }
  } else {
    return -1 + 2.09*v - 0.251 * v * v;
  }
  return -1;
}

float setDAC(int v) {
  dacWrite(25, v);
  float out = 3.3*(v/255.0);
  // corrections
  if(chipid == DEVICE_1){
    return 0.932*out + 0.0842;
  } else if(chipid == DEVICE_2){
    return 0.949*out + 0.0963;
  } else if(chipid == DEVICE_3){
    return 0.923*out + 0.123;
  }
  Serial.println("DEVICE NOT RECOGNIZED");
  return -1;
}

float thermistorCalc(float v) {
  float r = 10000.0*((3.3/v) - 1.0);
  // 3950 = beta, 298.15 = room temp
  float tKelvin = (3950 * 298.15) / 
            (3950 + (298.15 * log(r / 10000.0)));
  float tFah = (tKelvin - 273.15) * (9.0/5.0) + 32;
  return tFah;
}

void setup(void) {
  Serial.begin(115200);
  pinMode(23, OUTPUT);
  digitalWrite(23, HIGH);
  delay(200);
  
  chipid = ESP.getEfuseMac();
  if(chipid == DEVICE_1){
    Serial.println("DEVICE_1");
  } else if(chipid == DEVICE_2){
    Serial.println("DEVICE_2");
  } else if(chipid == DEVICE_3){
    Serial.println("DEVICE_3");
  } else {
    Serial.println("DEVICE NOT RECOGNIZED");
  }

  dht.setup(dhtPin, DHTesp::DHT11);
  TempAndHumidity tempAndHumidity = dht.getTempAndHumidity();
  
  String upper = String((uint16_t)(chipid>>32), HEX);
  String lower = String((uint32_t)chipid, HEX);
  String chipIdStr = "0x" + upper + lower;

  float a0 = readADC(A0);
  float a3 = readADC(A3);
  float a6 = readADC(A6);
  float a7 = readADC(A7);
  float a4 = readADC(A4);
  float a5 = readADC(A5);
  digitalWrite(23, LOW);
  
  String body = "{";
  body += "id: " + chipIdStr;
  body += ", a0: " + String(a0, 2);
  body += ", a3: " + String(a3, 2);
  body += ", a6: " + String(a6, 2);
  body += ", a7: " + String(a7, 2);
  body += ", a4: " + String(a4, 2);
  body += ", a5: " + String(a5, 2);
  body += ", t0: " + String(thermistorCalc(a0), 2);
  body += ", t3: " + String(thermistorCalc(a3), 2);
  body += ", t6: " + String(thermistorCalc(a6), 2);
  body += ", t7: " + String(thermistorCalc(a7), 2);
  body += ", t4: " + String(thermistorCalc(a4), 2);
  body += ", t5: " + String(thermistorCalc(a5), 2);
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

  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR * 60);
  delay(200);
  esp_deep_sleep_start();
}

void loop() {

}

