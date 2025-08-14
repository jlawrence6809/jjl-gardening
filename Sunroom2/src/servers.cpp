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
#include "../preact/build/static_files.h"
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include "rule_helpers.h"

bool POST_PARAM = true;
bool GET_PARAM = false;

AsyncWebServer server(80);

String JSON_CONTENT_TYPE = "application/json";
String PLAIN_TEXT_CONTENT_TYPE = "text/plain";
String HTML_CONTENT_TYPE = "text/html";

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
    std::map<String, String> relayMap;
    for (int i = 0; i < RELAY_PINS.size(); i++)
    {
        relayMap["relay_" + String(i)] = String(RELAY_VALUES[i]);
    }
    return buildJson(relayMap);
}

void getRelays(AsyncWebServerRequest *request)
{
    request->send(200, JSON_CONTENT_TYPE, getRelayValues());
}

void setRelays(AsyncWebServerRequest *request)
{
    for (int i = 0; i < RELAY_PINS.size(); i++)
    {
        String relayParam = "relay_" + String(i);
        if (request->hasParam(relayParam, POST_PARAM))
        {
            RELAY_VALUES[i] = static_cast<RelayValue>(request->getParam(relayParam, POST_PARAM)->value().toInt());
        }
    }

    processRelayRules();
    writeRelayValues();
    getRelays(request);
}

void handleWifiSettings(AsyncWebServerRequest *request)
{
    if (!request->hasParam("ssid", POST_PARAM) || !request->hasParam("password", POST_PARAM))
    {
        request->send(404, PLAIN_TEXT_CONTENT_TYPE, "Wifi Name or Wifi Password not found");
        return;
    }
    // Parameters are accessed differently in ESPAsyncWebServer
    SSID = request->getParam("ssid", POST_PARAM)->value();
    PASSWORD = request->getParam("password", POST_PARAM)->value();

    writeWifiCredentials(SSID, PASSWORD);

    request->send(200, PLAIN_TEXT_CONTENT_TYPE, "Wifi Name and Wifi Password updated. Restarting...");

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
    request->send(404, PLAIN_TEXT_CONTENT_TYPE, message);
}

void setupPreactPage()
{
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { 
                  AsyncWebServerResponse *response = request->beginResponse_P(200, HTML_CONTENT_TYPE, static_files::f_index_html_contents, static_files::f_index_html_size);
                  response->addHeader("Content-Encoding", "gzip");
                  request->send(response); });

    for (int i = 0; i < static_files::num_of_files; i++)
    {
        server.on(static_files::files[i].path, [i](AsyncWebServerRequest *request)
                  { 
                      AsyncWebServerResponse *response = request->beginResponse_P(200, static_files::files[i].type, static_files::files[i].contents, static_files::files[i].size);
                      response->addHeader("Content-Encoding", "gzip");
                      request->send(response); });
    }
}

void setupOTAUpdate()
{
    // OTA update
    server.on(
        "/update", HTTP_POST, [](AsyncWebServerRequest *request)
        {
            AsyncWebServerResponse *response = request->beginResponse(200, PLAIN_TEXT_CONTENT_TYPE, (Update.hasError()) ? "FAIL" : "OK");
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
    request->send(200, JSON_CONTENT_TYPE,
        buildJson({
            {"ChipId", String(CHIP_ID, HEX)},
            {"ResetCounter", String(RESET_COUNTER)},
            {"LastResetReason", String(LAST_RESET_REASON)},
            {"InternalTemperature", String(cToF(INTERNAL_CHIP_TEMPERATURE), 2)},
            {"CurrentTime", getLocalTimeString()},
            {"Core", String(xPortGetCoreID())},
            {"FreeHeap", String(FREE_HEAP)}
        })
    );
    // clang-format on
    Serial.println("GET /global-info done");
}

void getSensorInfo(AsyncWebServerRequest *request)
{
    Serial.println("GET /sensor-info");
    request->send(
        200,
        JSON_CONTENT_TYPE,
        buildJson({{"Temperature", String(cToF(CURRENT_TEMPERATURE), 2)},
                   {"Humidity", String(CURRENT_HUMIDITY, 2)},
                   {"ProbeTemperature", String(cToF(CURRENT_PROBE_TEMPERATURE), 2)},
                   {"Light", String(LIGHT_LEVEL)},
                   {"Switch", String(IS_SWITCH_ON)}}));

    Serial.println("GET /sensor-info done");
}

/**
 * Get the rules for a relay
 * call example: /rule?i=0
 */
void getRule(AsyncWebServerRequest *request)
{
    int relay = -1;
    if (request->hasParam("i", GET_PARAM))
    {
        relay = request->getParam("i", GET_PARAM)->value().toInt();
    }
    if (relay < 0 || relay >= RELAY_PINS.size())
    {
        request->send(404, JSON_CONTENT_TYPE, buildJson({{"Error", String("Relay not found")}}));
        return;
    }
    request->send(200, JSON_CONTENT_TYPE, buildJson({{"v", RELAY_RULES[relay]}}));
}

/**
 * Set the rules for a relay
 *
 * post example: /rule
 * formData:
 * rules: ["NOP"]
 * i: 0
 */
void setRule(AsyncWebServerRequest *request)
{
    // check for "i" and "v" parameters
    if (!request->hasParam("i", POST_PARAM) || !request->hasParam("v", POST_PARAM))
    {
        request->send(404, JSON_CONTENT_TYPE, buildJson({{"Error", String("Relay or rules not found")}}));
        return;
    }

    int relay = request->getParam("i", POST_PARAM)->value().toInt();
    if (relay < 0 || relay >= RELAY_PINS.size())
    {
        request->send(404, JSON_CONTENT_TYPE, buildJson({{"Error", String("Relay not found")}}));
        return;
    }
    String rules = request->getParam("v", POST_PARAM)->value();
    RELAY_RULES[relay] = rules;
    writeRelayRules();
    processRelayRules();
    request->send(200, JSON_CONTENT_TYPE, buildJson({{"v", RELAY_RULES[relay]}}));
}

void setRelayLabel(AsyncWebServerRequest *request)
{
    // check for "i" and "v" parameters
    if (!request->hasParam("i", POST_PARAM) || !request->hasParam("v", POST_PARAM))
    {
        request->send(404, JSON_CONTENT_TYPE, buildJson({{"Error", String("Relay or label not found")}}));
        return;
    }

    int relay = request->getParam("i", POST_PARAM)->value().toInt();
    if (relay < 0 || relay >= RELAY_PINS.size())
    {
        request->send(404, JSON_CONTENT_TYPE, buildJson({{"Error", String("Relay not found")}}));
        return;
    }
    String label = request->getParam("v", POST_PARAM)->value();
    RELAY_LABELS[relay] = label;
    writeRelayLabels();
    request->send(200, JSON_CONTENT_TYPE, buildJson({{"v", RELAY_LABELS[relay]}}));
}

void getRelayLabels(AsyncWebServerRequest *request)
{
    std::map<String, String> relayMap;
    for (int i = 0; i < RELAY_PINS.size(); i++)
    {
        relayMap["relay_" + String(i)] = RELAY_LABELS[i];
    }

    request->send(200, JSON_CONTENT_TYPE, buildJson(relayMap));
}

void onReset(AsyncWebServerRequest *request)
{
    // hardware reset
    request->send(200, JSON_CONTENT_TYPE, buildJson({{"ResetCounter", String(RESET_COUNTER)}}));
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
    server.on("/rule", HTTP_GET, getRule);
    server.on("/rule", HTTP_POST, setRule);
    server.on("/relay-labels", HTTP_GET, getRelayLabels);
    server.on("/relay-label", HTTP_POST, setRelayLabel);
    setupPreactPage();
    setupOTAUpdate();
    server.onNotFound(handleNotFound);

    server.begin();
}
