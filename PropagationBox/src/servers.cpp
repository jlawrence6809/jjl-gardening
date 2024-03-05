#include <Arduino.h>
#include <WebServer.h>
#include <Update.h>
#include "definitions.h"
#include "html_helpers.h"
#include "analog_helpers.h"
#include "time.h"
#include "preferences_helpers.h"
#include "time_helpers.h"
#include "json.h"
#include "../preact/build/static_files.h"

WebServer server(80);

/**
 * Parse time in format HH:MM to minutes
 */
int parseTimeAsMinutes(String hourMinute)
{
    int hour = hourMinute.substring(0, 2).toInt();
    int minute = hourMinute.substring(3, 5).toInt();
    return hour * 60 + minute;
}

void getSensorInfo()
{

    // clang-format off
    server.send(
        200,
        "application/json",
        buildJson({
        {"air_temperature", String(CURRENT_TEMPERATURE, 2)},
        {"humidity", String(CURRENT_HUMIDITY, 2)},
        {"probe_temperature", String(CURRENT_PROBE_TEMPERATURE, 2)},
        })
    );
}

void getPeripherals()
{
    server.send(
        200,
        "application/json",
        buildJson({
            {"heat_mat", String(IS_HEAT_MAT_ON ? "ON" : "OFF")},
            {"fan", String(IS_FAN_ON ? "ON" : "OFF")},
            {"led_level", String(LED_LEVEL * 100, 2)}
        })
    );
}

void getEnvironmentalControlValues()
{
    server.send(
        200,
        "application/json",
        buildJson({
            {"desired_temp", String(DESIRED_TEMPERATURE)},
            {"temp_range", String(TEMPERATURE_RANGE)},
            {"desired_humidity", String(DESIRED_HUMIDITY)},
            {"humidity_range", String(HUMIDITY_RANGE)},
            {"natural_light", String(USE_NATURAL_LIGHTING_CYCLE)},
            {"on_time", String(TURN_LIGHTS_ON_AT_MINUTE)},
            {"off_time", String(TURN_LIGHTS_OFF_AT_MINUTE)}
        })
    );
}

void setEnvironmentalControlValues()
{
    if (!server.hasArg("desired_temp") || !server.hasArg("temp_range") || !server.hasArg("desired_humidity") || !server.hasArg("humidity_range") || !server.hasArg("natural_light") || !server.hasArg("on_time") || !server.hasArg("off_time"))
    {
        server.send(404, "text/plain", "Desired temperature, Temperature Range, Desired humidity, Humidity Range, Natural light cycle, Turn on lights, or Turn off lights not found");
        return;
    }
    float desiredTemperature = server.arg("desired_temp").toFloat();
    float temperatureRange = server.arg("temp_range").toFloat();
    float desiredHumidity = server.arg("desired_humidity").toFloat();
    float humidityRange = server.arg("humidity_range").toFloat();
    bool useNaturalLightingCycle = server.arg("natural_light") == "1" ? true : false;
    int turnOnLightsAtMinute = server.arg("on_time").toInt();
    int turnOffLightsAtMinute = server.arg("off_time").toInt();

    DESIRED_TEMPERATURE = desiredTemperature;
    TEMPERATURE_RANGE = temperatureRange;
    DESIRED_HUMIDITY = desiredHumidity;
    HUMIDITY_RANGE = humidityRange;
    USE_NATURAL_LIGHTING_CYCLE = useNaturalLightingCycle;
    TURN_LIGHTS_ON_AT_MINUTE = turnOnLightsAtMinute;
    TURN_LIGHTS_OFF_AT_MINUTE = turnOffLightsAtMinute;

    writeEnvironmentalControlValues(
        desiredTemperature,
        temperatureRange,
        desiredHumidity,
        humidityRange,
        useNaturalLightingCycle,
        turnOnLightsAtMinute,
        turnOffLightsAtMinute);

    getEnvironmentalControlValues();
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

/**
 * Serve the preact page
*/
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

/**
 * Setup the OTA update endpoint
*/
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
            {"CurrentTime", getLocalTimeString()},
            {"Core", String(xPortGetCoreID())}
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
    server.on("/sensor-info", HTTP_GET, getSensorInfo);
    server.on("/peripherals", HTTP_GET, getPeripherals);
    server.on("/environmental-controls", HTTP_GET, getEnvironmentalControlValues);
    server.on("/environmental-controls", HTTP_POST, setEnvironmentalControlValues);
    server.on("/reset", HTTP_POST, onReset);
    server.onNotFound(handleNotFound);
    setupPreactPage();
    setupOTAUpdate();

    server.begin();
}
