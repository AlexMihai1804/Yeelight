// Example: Bulb Finder Demo
#include "Yeelight.h"
#include <WiFi.h>

#define WIFI_SSID "YourWiFiSSID"
#define WIFI_PASS "YourWiFiPassword"

void setup() {
    Serial.begin(115200);

    // Connect to WiFi (replace with your network credentials)
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("Connected to WiFi!");

    // Discover Yeelight bulbs
    std::vector<YeelightDevice> bulbs = Yeelight::discoverYeelightDevices(5000);

    if (bulbs.size() > 0) {
        Serial.println("Bulbs found:");
        for (YeelightDevice bulb: bulbs) {
            Serial.print("IP Address: ");
            Serial.print(bulb.ip[0]);
            Serial.print(".");
            Serial.print(bulb.ip[1]);
            Serial.print(".");
            Serial.print(bulb.ip[2]);
            Serial.print(".");
            Serial.println(bulb.ip[3]);

            Serial.print("Port: ");
            Serial.println(bulb.port);

            Serial.print("Model: ");
            Serial.println(bulb.model.c_str());

            Serial.print("Firmware version: ");
            Serial.println(bulb.fw_ver);

            Serial.print("Power: ");
            Serial.println(bulb.power ? "On" : "Off");

            Serial.print("Brightness: ");
            Serial.println(bulb.bright);

            Serial.print("Color temperature: ");
            Serial.println(bulb.ct);

            Serial.print("RGB: ");
            Serial.println(bulb.rgb);

            Serial.print("Hue: ");
            Serial.println(bulb.hue);

            Serial.print("Saturation: ");
            Serial.println(bulb.sat);

            Serial.print("Name: ");
            Serial.println(bulb.name.c_str());

            Serial.println("--------------------");
        }
    } else {
        Serial.println("No Yeelight bulbs found.");
    }
}

void loop() {
    // Nothing to do in the loop
}