#include <time.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ArduinoJSON.h>
#include <GxEPD.h>
#include <GxGDE0213B1/GxGDE0213B1.cpp>
#include <GxIO/GxIO_SPI/GxIO_SPI.cpp>
#include <GxIO/GxIO.cpp>
#include <Fonts/FreeMono9pt7b.h>

// Init e-paper display
GxIO_Class io(SPI, SS, D3, D4);
GxEPD_Class display(io);

// Sleep 900s. See ESP.DeepSleep function
#define SLEEP_SECONDS 90

// GraphQL que for Digitransit API. Get the stop-id's here: https://www.hsl.fi/reitit-ja-aikataulut
static const char digitransitQuery[] PROGMEM = "{\"query\":\"{stops(ids:[\\\"HSL:2215255\\\"]){name,stoptimesWithoutPatterns(numberOfDepartures:17){realtimeDeparture,realtime,trip{route{shortName}}}}}\"}";

// ArduinoJSON-buffer size. Has to be big enough for the answer.
// Check this: https://arduinojson.org/v5/assistant/
const size_t bufferSize = JSON_ARRAY_SIZE(1) + JSON_ARRAY_SIZE(16) + 32 * JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(2) + 15 * JSON_OBJECT_SIZE(3) + 1140;

void printTimetableRow(String busName, String departure, bool isRealtime, int idx) {
    /* 
    Prints bus-stop and the closest time 
    ect. 110T  21:34~
    */
    display.setCursor(2, 2 + idx * 14);
    display.print(busName);
    display.setCursor(54, 2 + idx * 14);
    display.print(departure);
    if (isRealtime)
    {
        display.setCursor(108, 2 + idx * 14);
        display.print("~");
    }
}

String parseTime(int seconds) {
    /* 
    Modifies the seconds into a HH:MM format
    ex. 78840 -> "21:54" 
    */
    int hours = seconds / 3600;
    int minutes = (seconds % 3600) / 60;
    char buffer[5];
    if (hours == 25)
        hours = 0;
    sprintf(buffer, "%02d:%02d", hours, minutes);
    return buffer;
}

void setup()
{
    Serial.begin(115200);

    // These are for your own wifi. I've omitted my own of couse.
    WiFi.begin("WIFINAME", "PASS");

    /*
    Config a static ip.
    ex.
    IPAddress ip(192,168,1,50);
    IPAddress gateway(192,168,1,1);
    IPAddress subnet(255,255,255,0);   
    WiFi.config(ip, gateway, subnet);
    */

    // Connects to wifi
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("connecting...");
        delay(250);
    }

    // NEXT: Create and send http-request for Digitransit API

    // Init http-client
    HTTPClient http; 

    // Huomaa kaksi vaihtoehtoista osoitetta Digitransitin rajapintoihin,
    // koko Suomen haku, ja HSL:n haku.

    // http.begin("http://api.digitransit.fi/routing/v1/routers/hsl/index/graphql"); // <- HSL
    http.begin("http://api.digitransit.fi/routing/v1/routers/finland/index/graphql"); // <- Finland wide 

    http.addHeader("Content-Type", "application/json"); // API requires JSON
    int httpCode = http.POST(digitransitQuery);         // POST request
    String payload = http.getString();                  // Save reply to payload
    http.end();

    // Make some parsing magic with objects
    DynamicJsonBuffer jsonBuffer(bufferSize);
    JsonObject &root = jsonBuffer.parseObject(payload.c_str());

    // otetaan referenssi JSON-muotoisen vastauksen bussilähdöistä 'departures'
    // Reference data and stops from 'payload'
    JsonArray &departures = root["data"]["stops"][0]["stoptimesWithoutPatterns"];

    // Init e-paper
    display.init();
    display.setTextColor(GxEPD_BLACK);
    display.setFont(&FreeMono9pt7b);

    Format values to something meaningful and add them to the buffer
    int idx = 0;
    for (auto &dep : departures)
    {
        int departureTime = dep["realtimeDeparture"];
        String departure = parseTime(departureTime);
        bool realTime = dep["realtime"]; // uses gps-tracking yes/no
        String busName = dep["trip"]["route"]["shortName"]; 
        printTimetableRow(busName, departure, realTime, ++idx);
    }

    // print or update buffer to the display
    display.update(); // Piirrä näyttöpuskurin sisältö E-paperinäytölle

    // make the controller sleep to save battery
    ESP.deepSleep(SLEEP_SECONDS * 1000000);
}

void loop() {
    // code never gets here because of the deepsleep.
}