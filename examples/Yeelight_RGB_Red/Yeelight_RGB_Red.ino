#include "Yeelight.h"
#include <WiFi.h>

const uint8_t ip[] = {192, 168, 1, 100};
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
    // Set the bulb color to red
    if (bulb.set_rgb_color(255, 0, 0) == ResponseType::SUCCESS) {
        Serial.println("Bulb color set to red.");
    } else {
        Serial.println("Error setting bulb color.");
    }
    delay(5000); // Wait for 5 seconds

    // Set the bulb brightness to 50%
    if (bulb.set_brightness(50) == ResponseType::SUCCESS) {
        Serial.println("Bulb brightness set to 50%.");
    } else {
        Serial.println("Error setting brightness.");
    }
    delay(5000); // Wait for 5 seconds

    // Turn off the bulb
    if (bulb.turn_off() == ResponseType::SUCCESS) {
        Serial.println("Bulb turned off.");
    } else {
        Serial.println("Error turning off bulb.");
    }
    delay(5000); // Wait for 5 seconds

    // Turn on the bulb again
    if (bulb.turn_on() == ResponseType::SUCCESS) {
        Serial.println("Bulb turned on.");
    } else {
        Serial.println("Error turning on bulb.");
    }
    delay(5000); // Wait for 5 seconds
}