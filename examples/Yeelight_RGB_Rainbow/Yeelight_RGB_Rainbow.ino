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
    // Set the bulb color to blue with 75% brightness
    if (bulb.set_rgb_color(0, 0, 255, 75) == ResponseType::SUCCESS) {
        Serial.println("Bulb set to blue with smooth transition.");
    } else {
        Serial.println("Error setting bulb color.");
    }
    delay(5000); // Wait for 5 seconds

    // Set the bulb color to green with sudden transition,
    // duration of 0.5 seconds
    if (bulb.set_rgb_color(0, 255, 0, EFFECT_SUDDEN, 500) == ResponseType::SUCCESS) {
        Serial.println("Bulb set to green with sudden transition.");
    } else {
        Serial.println("Error setting bulb color.");
    }
    delay(5000); // Wait for 5 seconds

    // Set the bulb color to yellow (using a different approach)
    if (bulb.set_rgb_color(255, 255, 0, EFFECT_SMOOTH, 1000) == ResponseType::SUCCESS) {
        Serial.println("Bulb set to yellow with smooth transition.");
    } else {
        Serial.println("Error setting bulb color.");
    }
    delay(5000); // Wait for 5 seconds

    // Set the bulb color to magenta with sudden transition and high brightness
    if (bulb.set_rgb_color(255, 0, 255, EFFECT_SUDDEN) == ResponseType::SUCCESS) {
        Serial.println("Bulb set to magenta with sudden transition and high brightness.");
    } else {
        Serial.println("Error setting bulb color.");
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