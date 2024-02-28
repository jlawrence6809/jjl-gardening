#include <Arduino.h>
#include <WebServer.h>
#include <Update.h>
#include "definitions.h"
#include "html_helpers.h"
#include "analog_helpers.h"
#include "time.h"
#include "preferences_helpers.h"
#include "time_helpers.h"

WebServer server(80);

String formatTime(int minutes)
{
    int hour = minutes / 60;
    int minute = minutes % 60;
    String hourString = hour < 10 ? "0" + String(hour) : String(hour);
    String minuteString = minute < 10 ? "0" + String(minute) : String(minute);
    return hourString + ":" + minuteString;
}

void handleRootGet()
{
    // float tFah = readTFahFromADC(A0);

    // String body = "{";
    // body += "a0: " + String(readADC(A0), 5);
    // body += ", a3: " + String(readADC(A3), 5);
    // body += ", a6: " + String(readADC(A6), 5);
    // body += ", a7: " + String(readADC(A7), 5);
    // body += ", a4: " + String(readADC(A4), 5);
    // body += ", a5: " + String(readADC(A5), 5);
    // body += ", tf: " + String(tFah);
    // // body += ", t: " + String(dht.toFahrenheit(tempAndHumidity.temperature), 0);d
    // // body += ", h: " + String(tempAndHumidity.humidity, 0);
    // body += " }";
    // Serial.println(body);
    // server.send(200, "application/json", body);

    // clang-format off
    String content = createPage(APP_NAME,
            createDiv("Chip Id: #" + String(CHIP_ID, HEX))
            + createDiv("Current temperature: " + String(CURRENT_TEMPERATURE, 2) + "C")
            + createBreak()
            + createDiv("Current humidity: " + String(CURRENT_HUMIDITY, 2) + "%")
            + createBreak()
            + createDiv("Current probe temperature: " + String(CURRENT_PROBE_TEMPERATURE, 2) + "C")
            + createBreak()
            + createDiv("Current time: " + getLocalTimeString())
            + createBreak()
            + createDivider()
            + createDiv("Heat mat: " + String(IS_HEAT_MAT_ON ? "ON" : "OFF"))
            + createBreak()
            + createDiv("Fan: " + String(IS_FAN_ON ? "ON" : "OFF"))
            + createBreak()
            + createDiv("LED level: " + String(LED_LEVEL * 100, 2) + "%")
            + createBreak()
            + createDivider()
            + createFormWithButton(
                createInputAndLabel("desired_temp", "Desired temperature (c)", "number", String(DESIRED_TEMPERATURE))
                + createBreak()
                + createInputAndLabel("temp_range", "Temperature range (c)", "number", String(TEMPERATURE_RANGE))
                + createBreak()
                + createInputAndLabel("desired_humidity", "Desired humidity (rh%)", "number", String(DESIRED_HUMIDITY))
                + createBreak()
                + createInputAndLabel("humidity_range", "Humidity range (rh%)", "number", String(HUMIDITY_RANGE))
                + createBreak()
                + createCheckboxInputAndLabel("natural_light", "Natural light cycle", USE_NATURAL_LIGHTING_CYCLE)
                + createBreak()
                + createInputAndLabel("on_time", "Turn on lights", "time", formatTime(TURN_LIGHTS_ON_AT_MINUTE))
                + createBreak()
                + createInputAndLabel("off_time", "Turn off lights", "time", formatTime(TURN_LIGHTS_OFF_AT_MINUTE))
                + createBreak(),
                "/"
            )
            + createDivider()
            + createFormWithButton(
                createInputAndLabel("ssid", "Wifi Name", "text", SSID)
                + createBreak()
                + createInputAndLabel("password", "Wifi Password", "password", PASSWORD)
                + createBreak(),
                "/wifi-settings"
            )
    );
    // clang-format on
    delay(100);
    server.send(200, "text/html", content);
}

/**
 * Parse time in format HH:MM to minutes
 */
int parseTimeAsMinutes(String hourMinute)
{
    int hour = hourMinute.substring(0, 2).toInt();
    int minute = hourMinute.substring(3, 5).toInt();
    return hour * 60 + minute;
}

void handleRootPost()
{
    // if (!server.hasArg("Desired temperature") || !server.hasArg("Desired humidity") || !server.hasArg("Natural light cycle") || !server.hasArg("Turn on lights") || !server.hasArg("Turn off lights"))
    if (!server.hasArg("desired_temp") || !server.hasArg("temp_range") || !server.hasArg("desired_humidity") || !server.hasArg("humidity_range") || !server.hasArg("on_time") || !server.hasArg("off_time"))
    {
        server.send(404, "text/plain", "Desired temperature, Temperature Range, Desired humidity, Humidity Range, Natural light cycle, Turn on lights, or Turn off lights not found");
        return;
    }
    float desiredTemperature = server.arg("desired_temp").toFloat();
    float temperatureRange = server.arg("temp_range").toFloat();
    float desiredHumidity = server.arg("desired_humidity").toFloat();
    float humidityRange = server.arg("humidity_range").toFloat();
    bool useNaturalLightingCycle = server.hasArg("natural_light");
    int turnOnLightsAtMinute = parseTimeAsMinutes(server.arg("on_time"));
    int turnOffLightsAtMinute = parseTimeAsMinutes(server.arg("off_time"));

    writeEnvironmentalControlValues(
        desiredTemperature,
        temperatureRange,
        desiredHumidity,
        humidityRange,
        useNaturalLightingCycle,
        turnOnLightsAtMinute,
        turnOffLightsAtMinute);

    server.send(200, "text/plain", "Desired temperature and Desired humidity updated. Restarting...");

    delay(1000);
    ESP.restart();
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

void handleADC()
{
    server.send(200, "text/plain", String(readADC(A0), 5));
}

void handleDAC()
{
    if (!server.hasArg("v"))
    {
        server.send(404, "text/plain", "v not found");
        return;
    }
    int v = server.arg("v").toInt();
    server.send(200, "text/plain", String(setDAC(v)));
}

void flipIOBit()
{
    if (!server.hasArg("pin"))
    {
        server.send(404, "text/plain", "pin not found");
        return;
    }
    String pin = server.arg("pin");
    Serial.println("flipping bit: " + pin);
    digitalWrite(pin.toInt(), !digitalRead(pin.toInt()));
    server.send(200, "text/plain", "flipped");
}

void checkIOBit()
{
    if (!server.hasArg("pin"))
    {
        server.send(404, "text/plain", "pin not found");
        return;
    }
    String pin = server.arg("pin");
    Serial.println("checking bit: " + pin);
    uint8_t val = digitalRead(pin.toInt());
    Serial.println("value: " + String(val));
    server.send(200, "text/plain", "value: " + String(val));
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
 * Setup the web server endpoints
 * Includes OTA update which can be run by curling like so:
 * curl -F "image=@Moisture.ino.node32s.bin" 192.168.29.215/update
 */
void serverSetup()
{
    server.on("/", HTTP_GET, handleRootGet);
    server.on("/", HTTP_POST, handleRootPost);
    server.on("/wifi-settings", HTTP_POST, handleWifiSettings);
    server.on("/flip", flipIOBit);
    server.on("/check", checkIOBit);
    server.on("/setdac", handleDAC);
    server.on("/readadc", handleADC);
    server.onNotFound(handleNotFound);

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

    server.begin();
}
