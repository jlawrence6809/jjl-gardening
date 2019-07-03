#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <DHTesp.h>
#include <Update.h>
#include <soc/efuse_reg.h>

// board "node32s"
// e fuse isn't written! (adc vref value)
uint64_t chipid;
uint64_t DEVICE_1 = 0xa85627a4ae30;
uint64_t DEVICE_2 = 0xc8216c12cfa4;
uint64_t DEVICE_3 = 0xe0286c12cfa4;

const char* ssid = "Jeremy";
const char* password = "3240winnie";

unsigned long check_wifi = 30000;

WebServer server(80);

// DHT stuff
DHTesp dht;
TaskHandle_t tempTaskHandle = NULL;
int dhtPin = 17;

// Thermistor stuff
#define THERM_SERIES 9900
#define THERM_NOM 10000

void handleADC() {
  server.send(200, "text/plain", String(readADC(A0), 5));
}

float readADC(int pin){
  int sum = 0;
  for(int i = 0; i < 10; i++){
    sum += analogRead(pin);
  }
  float v = sum / 10.0;
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

void handleRoot() {
//  TempAndHumidity tempAndHumidity = dht.getTempAndHumidity();

  float v = readADC(A0);
  float r = 10000.0*((3.3/v) - 1.0);
  // 3950 = beta, 298.15 = room temp
  float tKelvin = (3950 * 298.15) / 
            (3950 + (298.15 * log(r / 10000.0)));
  float tFah = (tKelvin - 273.15) * (9.0/5.0) + 32;
  
  String body = "{";
  body += "a0: " + String(readADC(A0), 5);
  body += ", a3: " + String(readADC(A3), 5);
  body += ", a6: " + String(readADC(A6), 5);
  body += ", a7: " + String(readADC(A7), 5);
  body += ", a4: " + String(readADC(A4), 5);
  body += ", a5: " + String(readADC(A5), 5);
  body += ", tf: " + String(tFah);
//  body += ", t: " + String(dht.toFahrenheit(tempAndHumidity.temperature), 0);d
//  body += ", h: " + String(tempAndHumidity.humidity, 0);
  body += " }";
  Serial.println(body);
  server.send(200, "application/json", body);
}

void handleDAC() {
  if(!server.hasArg("v")){
    server.send(404, "text/plain", "v not found");
    return;
  }
  int v = server.arg("v").toInt();
  server.send(200, "text/plain", String(setDAC(v)));
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

void flipIOBit() {
  if(!server.hasArg("pin")){
    server.send(404, "text/plain", "pin not found");
    return;
  }
  String pin = server.arg("pin");
  Serial.println("flipping bit: " + pin);
  server.send(200, "text/plain", "flipped");
}

void checkIOBit() {
  if(!server.hasArg("pin")){
    server.send(404, "text/plain", "pin not found");
    return;
  }
  String pin = server.arg("pin");
  Serial.println("checking bit: " + pin);
  server.send(200, "text/plain", "checked");
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
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp32")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);
  server.on("/flip", flipIOBit);
  server.on("/check", checkIOBit);
  server.on("/setdac", handleDAC);
  server.on("/readadc", handleADC);
  server.onNotFound(handleNotFound);

  // cd ~/code/arduino-sketchbook/Moisture 
  // curl -F "image=@Moisture.ino.node32s.bin" 192.168.29.215/update
  server.on("/update", HTTP_POST, []() {
      server.sendHeader("Connection", "close");
      server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
      ESP.restart();
    }, []() {
      HTTPUpload& upload = server.upload();
      if (upload.status == UPLOAD_FILE_START) {
        Serial.printf("Update: %s\n", upload.filename.c_str());
        if (!Update.begin()) { //start with max available size
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) { //true to set the size to the current progress
          Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
        } else {
          Update.printError(Serial);
        }
        Serial.setDebugOutput(false);
      }
    });

  server.begin();
  Serial.println("HTTP server started");

  dht.setup(dhtPin, DHTesp::DHT11);
  Serial.println("DHT setup");
}

void loop() {
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
  }
}

