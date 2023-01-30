/*
Handle wifi endpoints and setup
*/

#include "Arduino.h"
#include <cstdint>

#include <core.h>

// Include the WiFi Library
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <LEAmDNS.h>
#include <CircularBuffer.h>
#include <SD.h>

extern Config config;

CircularBuffer<String, 200> datalogBuffer;

// Filename to read data from
const char *serverFilename = "/data_raw.jsonfiles";

// Maximum time to read sd file data
const int dataReadTimeout = 10000;

// Replace with your network credentials
int port = 80;

WiFiMulti multi;
WebServer server(port);

static const char TEXT_PLAIN[] PROGMEM = "text/plain";
static const char FS_INIT_ERROR[] PROGMEM = "FS INIT ERROR";
static const char FILE_NOT_FOUND[] PROGMEM = "FileNotFound";

////////////////////////////////
// Utils to return HTTP codes, and determine content-type

void replyOK()
{
    Serial.println(F("OK"));
    server.send(200, FPSTR(TEXT_PLAIN), "");
}

void replyOKWithMsg(String msg)
{
    Serial.println(F("OK"));
    server.send(200, FPSTR(TEXT_PLAIN), msg);
}

void replyNotFound(String msg)
{
    Serial.println(msg);
    server.send(404, FPSTR(TEXT_PLAIN), msg);
}

void replyBadRequest(String msg)
{
    Serial.println(msg);
    server.send(400, FPSTR(TEXT_PLAIN), msg + "\r\n");
}

void replyServerError(String msg)
{
    Serial.println(msg);
    server.send(500, FPSTR(TEXT_PLAIN), msg + "\r\n");
}

const String postForms = "<html>\
  <head>\
    <title>Pico-W Web Server POST handling</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>POST plain text to /postplain/</h1><br>\
    <form method=\"post\" enctype=\"text/plain\" action=\"/postplain/\">\
      <input type=\"text\" name=\'{\"hello\": \"world\", \"trash\": \"\' value=\'\"}\'><br>\
      <input type=\"submit\" value=\"Submit\">\
    </form>\
    <h1>POST form data to /postform/</h1><br>\
    <form method=\"post\" enctype=\"application/x-www-form-urlencoded\" action=\"/postform/\">\
      <input type=\"text\" name=\"hello\" value=\"world\"><br>\
      <input type=\"submit\" value=\"Submit\">\
    </form>\
  </body>\
</html>";

const String testingJSON = "{\"time_rtc\":1666261732,\"eCO2_sgp30\":0,\"TVOC_sgp30\":0,\"altitude_bmp280\":173.3957977,\"pressure_bmp280\":99259.59375,\"temperature_bmp280\":24.19000053,\"temperature_aht20\":23.21147919,\"humidity_aht20\":52.9419899,\"CO2_SCD40\":661,\"temperature_SCD40\":26.80671692,\"humidity_SCD40\":42.20275879}\n{\"time_rtc\":1666261732,\"eCO2_sgp30\":0,\"TVOC_sgp30\":0,\"altitude_bmp280\":173.3957977,\"pressure_bmp280\":99259.59375,\"temperature_bmp280\":24.19000053,\"temperature_aht20\":23.21147919,\"humidity_aht20\":52.9419899,\"CO2_SCD40\":661,\"temperature_SCD40\":26.80671692,\"humidity_SCD40\":42.20275879}\n{\"time_rtc\":1666261732,\"eCO2_sgp30\":0,\"TVOC_sgp30\":0,\"altitude_bmp280\":173.3957977,\"pressure_bmp280\":99259.59375,\"temperature_bmp280\":24.19000053,\"temperature_aht20\":23.21147919,\"humidity_aht20\":52.9419899,\"CO2_SCD40\":661,\"temperature_SCD40\":26.80671692,\"humidity_SCD40\":42.20275879}\n{\"time_rtc\":1666261732,\"eCO2_sgp30\":0,\"TVOC_sgp30\":0,\"altitude_bmp280\":173.3957977,\"pressure_bmp280\":99259.59375,\"temperature_bmp280\":24.19000053,\"temperature_aht20\":23.21147919,\"humidity_aht20\":52.9419899,\"CO2_SCD40\":661,\"temperature_SCD40\":26.80671692,\"humidity_SCD40\":42.20275879}";

const int led = LED_BUILTIN;

void handlePlain()
{
    if (server.method() != HTTP_POST)
    {
        digitalWrite(led, 1);
        server.send(405, "text/plain", "Method Not Allowed");
        digitalWrite(led, 0);
    }
    else
    {
        digitalWrite(led, 1);
        server.send(200, "text/plain", "POST body was:\n" + server.arg("plain"));
        digitalWrite(led, 0);
    }
}

void handleForm()
{
    if (server.method() != HTTP_POST)
    {
        digitalWrite(led, 1);
        server.send(405, "text/plain", "Method Not Allowed");
        digitalWrite(led, 0);
    }
    else
    {
        digitalWrite(led, 1);
        String message = "POST form was:\n";
        for (uint8_t i = 0; i < server.args(); i++)
        {
            message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
        }
        server.send(200, "text/plain", message);
        digitalWrite(led, 0);
    }
}

void handleNotFound()
{
    digitalWrite(led, 1);
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
    digitalWrite(led, 0);
}

void returnStaticFile()
{
    digitalWrite(led, 1);
    Serial.println("reading static file");

    File file = SD.open(serverFilename);
    file.setTimeout(dataReadTimeout);
    if (!file)
    {
        Serial.println(F("Failed to open file"));
        digitalWrite(led, 0);
        return;
    }
    // read from the file until there's nothing else in it:
    String message = "";
    while (file.available())
    {
        String dataString = file.readString();
        message += dataString;
    }
    // close the file:
    file.close();

    server.sendHeader(F("Access-Control-Allow-Origin"), "*");
    server.send(200, "text/plain", message);
    digitalWrite(led, 0);
}

void handleRawData()
{
    digitalWrite(led, 1);
    Serial.println("reading data for raw output");
    Serial.println("free mem before read:");
    Serial.println(rp2040.getFreeHeap());

    File file = SD.open(serverFilename);
    file.setTimeout(dataReadTimeout);
    if (!file)
    {
        Serial.println(F("Failed to open file"));
        digitalWrite(led, 0);
        return;
    }
    // read from the file until there's nothing else in it:
    String message = "";
    while (file.available())
    {
        String dataString = file.readStringUntil('\n');
        datalogBuffer.push(dataString);
    }
    // close the file:
    file.close();

    Serial.println("free mem after read:");
    Serial.println(rp2040.getFreeHeap());
    Serial.println("copying to response message");
    server.sendHeader(F("Access-Control-Allow-Origin"), "*");

    // the following ensures using the right type for the index variable
    using index_t = decltype(datalogBuffer)::index_t;
    for (index_t i = 0; i < datalogBuffer.size(); i++)
    {
        message += datalogBuffer[i];
        message += "\n";
    }

    server.send(200, "text/plain", message);
    digitalWrite(led, 0);
}

bool handleFileRead(String path = "")
{
    digitalWrite(led, 1);
    if (path == "")
    {
        digitalWrite(led, 0);
        replyBadRequest(F("PATH ARG MISSING"));
        return false;
    }

    Serial.println(String("handleFileRead: ") + path);
    File file = SD.open(path, "r");
    if (!file)
    {
        Serial.println(F("Failed to open file"));
        digitalWrite(led, 0);
        return false;
    }

    String contentType = mime::getContentType(path);
    server.sendHeader(F("Access-Control-Allow-Origin"), "*");
    // if (!server.chunkedResponseModeStart(200, "text/json"))
    // {
    //     server.send(505, F("text/html"), F("HTTP1.1 required"));
    //     return false;
    // }
    char buf[1024];
    int siz = file.size();
    while (siz > 0)
    {
        size_t len = std::min((int)(sizeof(buf) - 1), siz);
        file.read((uint8_t *)buf, len);
        server.client().write((const char *)buf, len);
        siz -= len;
    }

    // unsigned long streamedLength = server.streamFile(file, contentType);
    // char streamedLengthChar[40];
    // sprintf(streamedLengthChar, "%lu", streamedLength);
    // if (streamedLength != file.size())
    // {
    //     Serial.println(String("Sent less data than expected! "));
    //     Serial.println(streamedLength);
    //     Serial.println(file.size());
    //     // server.sendHeader(F("Content-Length"), streamedLengthChar);
    // }
    file.close();
    Serial.println(F("Returned File"));
    // server.send(200, "text/plain");
    digitalWrite(led, 0);
    return true;
}

void handleStaticFile()
{
    String path = server.arg("path");
    handleFileRead(String("/static/") + path);
}

bool handleRoot()
{
    return handleFileRead(String("/static/") + "viewData.html");
}

bool handleConfig()
{
    return handleFileRead("config.json");
}

bool setupServer()
{
    Serial.print("Connecting to Wifi");

    // Start WiFi with supplied parameters
    multi.addAP(config.ssid, config.password);

    int wifiAttempts = 0;
    int maxWifiAttempts = 30;
    // Print periods on monitor while establishing connection
    while (multi.run() != WL_CONNECTED)
    {
        delay(1000);
        Serial.print(".");
        wifiAttempts++;
        if (wifiAttempts > maxWifiAttempts) {
            Serial.println("Timed out connecting to wifi");
            return false;
        }
    }

    // Connection established
    Serial.println("");
    Serial.print("Pico W is connected to WiFi network ");
    Serial.println(WiFi.SSID());

    // Print IP Address
    Serial.print("Assigned IP Address: ");
    Serial.println(WiFi.localIP());

    if (MDNS.begin(config.mDNSName))
    {
        Serial.println(String("MDNS responder started with name: ") + config.mDNSName);
    }

    // Serve files
    server.on("/static/", handleStaticFile);

    server.on("/", handleRoot);
    server.on("/postplain/", handlePlain);
    server.on("/postform/", handleForm);
    server.on("/rawdata/", handleRawData);
    server.on("/config/", handleConfig);

    server.onNotFound(handleNotFound);
    server.begin();
    Serial.println("HTTP server started");
    return true;
}

void loopServer()
{
    server.handleClient();
    MDNS.update();
}
