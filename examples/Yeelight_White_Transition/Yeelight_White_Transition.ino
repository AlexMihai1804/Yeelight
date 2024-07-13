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
    // Turn on the bulb to 2700K with smooth transition and duration of 1 second
    if (bulb.set_color_temp(2700, EFFECT_SMOOTH, 1000) == ResponseType::SUCCESS) {
        Serial.println("Bulb set to 2700K with smooth transition.");
    } else {
        Serial.println("Error setting color temperature.");
    }
    delay(5000); // Wait for 5 seconds

    // Turn on the bulb to 6500K with sudden transition and duration of 0.5 seconds
    if (bulb.set_color_temp(6500, EFFECT_SUDDEN, 500) == ResponseType::SUCCESS) {
        Serial.println("Bulb set to 6500K with sudden transition.");
    } else {
        Serial.println("Error setting color temperature.");
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