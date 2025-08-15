#ifndef UNIT_TEST
#include <Arduino.h>
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
#include "system_info.h"
#include "units.h"
#include "peripheral_controls.h"
#include "pin_helpers.h"
#include <set>
#include <vector>
#include <algorithm>

bool POST_PARAM = true;
bool GET_PARAM = false;

AsyncWebServer server(80);

String JSON_CONTENT_TYPE = "application/json";
String PLAIN_TEXT_CONTENT_TYPE = "text/plain";
String HTML_CONTENT_TYPE = "text/html";

 

String getRelayValues()
{
    std::map<String, String> relayMap;
    for (int i = 0; i < RUNTIME_RELAY_COUNT; i++)
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
    for (int i = 0; i < RUNTIME_RELAY_COUNT; i++)
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
    request->send(200, JSON_CONTENT_TYPE, systemInfoJson());
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
    if (relay < 0 || relay >= RUNTIME_RELAY_COUNT)
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
    if (relay < 0 || relay >= RUNTIME_RELAY_COUNT)
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
    if (relay < 0 || relay >= RUNTIME_RELAY_COUNT)
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
    for (int i = 0; i < RUNTIME_RELAY_COUNT; i++)
    {
        relayMap["relay_" + String(i)] = RELAY_LABELS[i];
    }

    request->send(200, JSON_CONTENT_TYPE, buildJson(relayMap));
}

// List available GPIO options (valid and not reserved and not already used)
void getGpioOptions(AsyncWebServerRequest *request)
{
    std::set<int> used;
    for (int i = 0; i < RUNTIME_RELAY_COUNT; i++) used.insert(RUNTIME_RELAY_PINS[i]);
    std::map<String, String> opts;
    for (int pin : VALID_GPIO_PINS)
    {
        if (used.count(pin)) continue;
        opts[String(pin)] = String(pin);
    }
    request->send(200, JSON_CONTENT_TYPE, buildJson(opts));
}

// Get current relay config (pins and inversion)
void getRelayConfig(AsyncWebServerRequest *request)
{
    DynamicJsonDocument doc(1024);
    doc["count"] = RUNTIME_RELAY_COUNT;
    JsonArray pins = doc.createNestedArray("pins");
    JsonArray inv = doc.createNestedArray("inverted");
    for (int i = 0; i < RUNTIME_RELAY_COUNT; i++) { pins.add(RUNTIME_RELAY_PINS[i]); inv.add(RUNTIME_RELAY_IS_INVERTED[i]); }
    String out; serializeJson(doc, out);
    request->send(200, JSON_CONTENT_TYPE, out);
}

// Add a relay: expects form fields pin=<int>&inv=<0|1>
void addRelay(AsyncWebServerRequest *request)
{
    if (!request->hasParam("pin", POST_PARAM))
    {
        request->send(400, JSON_CONTENT_TYPE, buildJson({{"Error", String("pin required")}}));
        return;
    }
    int pin = request->getParam("pin", POST_PARAM)->value().toInt();
    bool inv = request->hasParam("inv", POST_PARAM) ? (request->getParam("inv", POST_PARAM)->value().toInt() == 1) : true;
    if (RUNTIME_RELAY_COUNT >= MAX_RELAYS)
    {
        request->send(400, JSON_CONTENT_TYPE, buildJson({{"Error", String("max relays reached")}}));
        return;
    }
    if (std::find(VALID_GPIO_PINS.begin(), VALID_GPIO_PINS.end(), pin) == VALID_GPIO_PINS.end())
    {
        request->send(400, JSON_CONTENT_TYPE, buildJson({{"Error", String("invalid pin")}}));
        return;
    }
    for (int i = 0; i < RUNTIME_RELAY_COUNT; i++) { if (RUNTIME_RELAY_PINS[i] == pin) { request->send(400, JSON_CONTENT_TYPE, buildJson({{"Error", String("pin already used")}})); return; } }

    int idx = RUNTIME_RELAY_COUNT;
    RUNTIME_RELAY_PINS[idx] = pin;
    RUNTIME_RELAY_IS_INVERTED[idx] = inv;
    RELAY_VALUES[idx] = FORCE_OFF_AUTO_X;
    RELAY_RULES[idx] = "[\"NOP\"]";
    RELAY_LABELS[idx] = String("Relay ") + String(idx);
    RUNTIME_RELAY_COUNT++;
    writeRelayConfig();
    writeRelayValues();
    writeRelayRules();
    writeRelayLabels();
    setupRelays();
    getRelayConfig(request);
}

// Remove last relay (or by index if provided: i=<int>)
void removeRelay(AsyncWebServerRequest *request)
{
    int idx = RUNTIME_RELAY_COUNT - 1;
    if (request->hasParam("i", POST_PARAM)) idx = request->getParam("i", POST_PARAM)->value().toInt();
    if (idx < 0 || idx >= RUNTIME_RELAY_COUNT)
    {
        request->send(400, JSON_CONTENT_TYPE, buildJson({{"Error", String("invalid index")}}));
        return;
    }
    // shift left from idx
    for (int i = idx; i < RUNTIME_RELAY_COUNT - 1; i++)
    {
        RUNTIME_RELAY_PINS[i] = RUNTIME_RELAY_PINS[i+1];
        RUNTIME_RELAY_IS_INVERTED[i] = RUNTIME_RELAY_IS_INVERTED[i+1];
        RELAY_VALUES[i] = RELAY_VALUES[i+1];
        RELAY_RULES[i] = RELAY_RULES[i+1];
        RELAY_LABELS[i] = RELAY_LABELS[i+1];
    }
    RUNTIME_RELAY_COUNT--;
    writeRelayConfig();
    writeRelayValues();
    writeRelayRules();
    writeRelayLabels();
    setupRelays();
    getRelayConfig(request);
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
    server.on("/gpio-options", HTTP_GET, getGpioOptions);
    server.on("/relay-config", HTTP_GET, getRelayConfig);
    server.on("/relay-config/add", HTTP_POST, addRelay);
    server.on("/relay-config/remove", HTTP_POST, removeRelay);
    
    setupPreactPage();
    setupOTAUpdate();
    server.onNotFound(handleNotFound);

    server.begin();
}

#endif // UNIT_TEST