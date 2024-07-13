#include "Yeelight.h"
#include <WiFi.h>

const uint8_t ip[] = {192, 168, 1, 100}; // IP address of the bulb
Yeelight bulb;

void setup() {
    Serial.begin(115200);

    // Connect to WiFi (replace with your network credentials)
    WiFi.begin("YourWiFiSSID", "YourWiFiPassword");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("Connected to WiFi!");

    // Connect to the bulb
    if (bulb.connect(ip) == ResponseType::SUCCESS) {
        Serial.println("Connected to Yeelight bulb.");
    } else {
        Serial.println("Error connecting to bulb.");
    }
}

void loop() {
    if (bulb.turn_on() == ResponseType::SUCCESS) {
        Serial.println("Bulb turned on.");
        delay(500); // Wait for 0.5 seconds
    } else {
        Serial.println("Error turning on bulb.");
    }

    if (bulb.turn_off() == ResponseType::SUCCESS) {
        Serial.println("Bulb turned off.");
        delay(500); // Wait for 0.5 seconds
    } else {
        Serial.println("Error turning off bulb.");
    }
}