#include <Arduino.h>
#include <WebServer.h>
#include <Update.h>
#include "definitions.h"
#include "analog_helpers.h"
#include "time.h"
#include "preferences_helpers.h"
#include "time_helpers.h"
#include <map>
#include "json.h"
// #include "../preact/build/static_files.h"
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>

AsyncWebServer server(80);

/**
 * Convert celsius to fahrenheit
 */
float cToF(float c)
{
    if (c == NULL_TEMPERATURE)
    {
        return NULL_TEMPERATURE;
    }
    return c * 9 / 5 + 32;
}

String getRelayValues()
{
    return buildJson({
        {"relay_1", String(RELAY_VALUES[RELAY_1_PIN])},
        {"relay_2", String(RELAY_VALUES[RELAY_2_PIN])},
        {"relay_3", String(RELAY_VALUES[RELAY_3_PIN])},
        {"relay_4", String(RELAY_VALUES[RELAY_4_PIN])},
        {"relay_5", String(RELAY_VALUES[RELAY_5_PIN])},
        {"relay_6", String(RELAY_VALUES[RELAY_6_PIN])},
        {"relay_7", String(RELAY_VALUES[RELAY_7_PIN])},
        {"relay_8", String(RELAY_VALUES[RELAY_8_PIN])},
    });
}

void getRelays(AsyncWebServerRequest *request)
{
    request->send(200, "application/json", getRelayValues());
}

void setRelay(AsyncWebServerRequest *request, String arg, int pin)
{
    if (!request->hasParam(arg, true))
    {
        return;
    }
    bool relay = request->getParam(arg, true)->value().toInt() > 0;
    digitalWrite(pin, relay);
    RELAY_VALUES[pin] = relay;
}

void setRelays(AsyncWebServerRequest *request)
{
    setRelay(request, "relay_1", RELAY_1_PIN);
    setRelay(request, "relay_2", RELAY_2_PIN);
    setRelay(request, "relay_3", RELAY_3_PIN);
    setRelay(request, "relay_4", RELAY_4_PIN);
    setRelay(request, "relay_5", RELAY_5_PIN);
    setRelay(request, "relay_6", RELAY_6_PIN);
    setRelay(request, "relay_7", RELAY_7_PIN);
    setRelay(request, "relay_8", RELAY_8_PIN);
    writeRelayValues();
    getRelays(request);
}

// void handleWifiSettings(AsyncWebServerRequest *request)
// {
//     if (!server.hasArg("ssid") || !server.hasArg("password"))
//     {
//         request->send(404, "text/plain", "Wifi Name or Wifi Password not found");
//         return;
//     }
//     SSID = server.arg("ssid");
//     PASSWORD = server.arg("password");

//     writeWifiCredentials(SSID, PASSWORD);

//     request->send(200, "text/plain", "Wifi Name and Wifi Password updated. Restarting...");

//     delay(1000);
//     ESP.restart();
// }
void handleWifiSettings(AsyncWebServerRequest *request)
{
    if (!request->hasParam("ssid", true) || !request->hasParam("password", true))
    {
        request->send(404, "text/plain", "Wifi Name or Wifi Password not found");
        return;
    }
    // Parameters are accessed differently in ESPAsyncWebServer
    SSID = request->getParam("ssid", true)->value();
    PASSWORD = request->getParam("password", true)->value();

    writeWifiCredentials(SSID, PASSWORD);

    request->send(200, "text/plain", "Wifi Name and Wifi Password updated. Restarting...");

    delay(1000);
    ESP.restart();
}

void handleNotFound(AsyncWebServerRequest *request)
{
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += request->url();
    message += "\nMethod: ";
    message += (request->method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += request->args();
    message += "\n";
    for (uint8_t i = 0; i < request->args(); i++)
    {
        message += " " + request->argName(i) + ": " + request->arg(i) + "\n";
    }
    request->send(404, "text/plain", message);
}

// // async version:
// void setupPreactPage()
// {
//     // Optional, defines the default entrypoint
//     server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
//               { request->send_P(200, "text/html", static_files::f_index_html_contents, static_files::f_index_html_size); });

//     // Create a route handler for each of the build artifacts
//     for (int i = 0; i < static_files::num_of_files; i++)
//     {
//         server.on(static_files::files[i].path, [i](AsyncWebServerRequest *request)
//                   { request->send_P(200, static_files::files[i].type, static_files::files[i].contents, static_files::files[i].size); });
//     }
// }

// async version:
void setupOTAUpdate()
{

    // Rest of your code...

    // OTA update
    server.on(
        "/update", HTTP_POST, [](AsyncWebServerRequest *request)
        {
            AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
            response->addHeader("Connection", "close");
            request->send(response);
            ESP.restart(); },
        [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
        {
            if (!index)
            {
                Serial.printf("Update: %s\n", filename.c_str());
                if (!Update.begin(UPDATE_SIZE_UNKNOWN))
                { // start with max available size
                    Update.printError(Serial);
                }
            }
            if (Update.write(data, len) != len)
            {
                Update.printError(Serial);
            }
            if (final)
            {
                if (Update.end(true))
                { // true to set the size to the current progress
                    Serial.printf("Update Success: %u\nRebooting...\n", index + len);
                }
                else
                {
                    Update.printError(Serial);
                }
                Serial.setDebugOutput(false);
            }
        });
}

void getGlobalInfo(AsyncWebServerRequest *request)
{
    // clang-format off
    request->send(
        200,
        "application/json",
        buildJson({
            {"ChipId", String(CHIP_ID, HEX)},
            {"ResetCounter", String(RESET_COUNTER)},
            {"InternalTemperature", String(cToF(INTERNAL_CHIP_TEMPERATURE), 2)},
            {"CurrentTime", getLocalTimeString()},
            {"Core", String(xPortGetCoreID())}
        })
    );
}

void getSensorInfo(AsyncWebServerRequest *request)
{
    int light = analogRead(PHOTO_SENSOR_PIN);
    int switchV = digitalRead(LIGHT_SWITCH_PIN);
    request->send(
        200,
        "application/json",
        buildJson({
            {"Temperature", String(cToF(CURRENT_TEMPERATURE), 2)},
            {"Humidity", String(CURRENT_HUMIDITY, 2)},
            {"ProbeTemperature", String(cToF(CURRENT_PROBE_TEMPERATURE), 2)},
            {"Light", String(light)},
            {"Switch", String(switchV)}
        })
    );
}

// void getRules(AsyncWebServerRequest *request)
// {
//     // request->send(200, "application/json", getRulesJson());
// }

// void setRules(AsyncWebServerRequest *request)
// {
//     // parse json payload
//     DynamicJsonDocument doc(512);
//     deserializeJson(doc, server.arg("plain"));

// }

void onReset(AsyncWebServerRequest *request)
{
    // hardware reset
    request->send(200, "application/json", buildJson({{"ResetCounter", String(RESET_COUNTER)}}));
    delay(200);
    ESP.restart();
}

/**
 * Setup the web server endpoints
 * Includes OTA update which can be run by curling like so:
 * curl -F "image=@Moisture.ino.node32s.bin" 192.168.29.215/update
 */
void serverSetup()
{
    server.on("/global-info", HTTP_GET, getGlobalInfo);
    server.on("/wifi-settings", HTTP_POST, handleWifiSettings);
    server.on("/relays", HTTP_GET, getRelays);
    server.on("/relays", HTTP_POST, setRelays);
    server.on("/sensor-info", HTTP_GET, getSensorInfo);
    server.on("/reset", HTTP_POST, onReset);
    // server.on("/rules", HTTP_GET, getRules);
    // server.on("/rules", HTTP_POST, setRules);
    server.onNotFound(handleNotFound);
    // setupPreactPage();
    setupOTAUpdate();

    server.begin();
}
