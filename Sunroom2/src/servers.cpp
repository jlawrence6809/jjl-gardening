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

WebServer server(80);

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

void getRelays()
{
    server.send(200, "application/json", getRelayValues());
}

void setRelay(String arg, int pin)
{
    if (!server.hasArg(arg))
    {
        return;
    }
    bool relay = server.arg(arg).toInt() > 0;
    digitalWrite(pin, relay);
    RELAY_VALUES[pin] = relay;
}

void setRelays()
{
    setRelay("relay_1", RELAY_1_PIN);
    setRelay("relay_2", RELAY_2_PIN);
    setRelay("relay_3", RELAY_3_PIN);
    setRelay("relay_4", RELAY_4_PIN);
    setRelay("relay_5", RELAY_5_PIN);
    setRelay("relay_6", RELAY_6_PIN);
    setRelay("relay_7", RELAY_7_PIN);
    setRelay("relay_8", RELAY_8_PIN);
    writeRelayValues();
    getRelays();
}

void handleWifiSettings()
{
    if (!server.hasArg("ssid") || !server.hasArg("password"))
    {
        server.send(404, "text/plain", "Wifi Name or Wifi Password not found");
        return;
    }
    SSID = server.arg("ssid");
    PASSWORD = server.arg("password");

    writeWifiCredentials(SSID, PASSWORD);

    server.send(200, "text/plain", "Wifi Name and Wifi Password updated. Restarting...");

    delay(1000);
    ESP.restart();
}

void handleNotFound()
{
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += (server.method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";
    for (uint8_t i = 0; i < server.args(); i++)
    {
        message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
    server.send(404, "text/plain", message);
}

void serverLoop()
{
    server.handleClient();
}

void setupPreactPage()
{

    // Optional, defines the default entrypoint
    server.on("/", HTTP_GET, []
              {
            server.sendHeader("Content-Encoding", "gzip");
            server.send_P(200, "text/html", (const char *)static_files::f_index_html_contents, static_files::f_index_html_size); });

    // Create a route handler for each of the build artifacts
    for (int i = 0; i < static_files::num_of_files; i++)
    {
        server.on(static_files::files[i].path, [i]
                  {
            server.sendHeader("Content-Encoding", "gzip");
            server.send_P(200, static_files::files[i].type, (const char *)static_files::files[i].contents, static_files::files[i].size); });
    }
}

void setupOTAUpdate()
{
    // OTA update
    server.on(
        "/update", HTTP_POST, []()
        {
      server.sendHeader("Connection", "close");
      server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
      ESP.restart(); },
        []()
        {
            HTTPUpload &upload = server.upload();
            if (upload.status == UPLOAD_FILE_START)
            {
                Serial.printf("Update: %s\n", upload.filename.c_str());
                if (!Update.begin())
                { // start with max available size
                    Update.printError(Serial);
                }
            }
            else if (upload.status == UPLOAD_FILE_WRITE)
            {
                if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
                {
                    Update.printError(Serial);
                }
            }
            else if (upload.status == UPLOAD_FILE_END)
            {
                if (Update.end(true))
                { // true to set the size to the current progress
                    Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
                }
                else
                {
                    Update.printError(Serial);
                }
                Serial.setDebugOutput(false);
            }
        });
}

void getGlobalInfo()
{
    // clang-format off
    server.send(
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

void getSensorInfo()
{
    int light = analogRead(PHOTO_SENSOR_PIN);
    int switchV = digitalRead(LIGHT_SWITCH_PIN);
    server.send(
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

void onReset()
{
    // hardware reset
    server.send(200, "application/json", buildJson({{"ResetCounter", String(RESET_COUNTER)}}));
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
    server.onNotFound(handleNotFound);
    setupPreactPage();
    setupOTAUpdate();

    server.begin();
}
