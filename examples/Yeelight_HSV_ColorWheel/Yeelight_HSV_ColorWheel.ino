// Example: HSV Color Wheel Demo
#include <Yeelight.h>
#include <WiFi.h>

#define WIFI_SSID "YourWiFiSSID"
#define WIFI_PASS "YourWiFiPassword"
const uint8_t bulbIP[] = {192, 168, 1, 100};

Yeelight bulb;

void setup() {
    Serial.begin(115200);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("Connected to WiFi!");
    if (bulb.connect(bulbIP) != ResponseType::SUCCESS) {
        Serial.println("Error connecting to bulb.");
        return;
    }
    Serial.println("Connected to Yeelight bulb.");
}

void loop() {
    // Set the bulb color to yellow with smooth transition,
    // duration of 1 second
    if (bulb.set_hsv_color(60, 100, EFFECT_SMOOTH, 1000) == ResponseType::SUCCESS) {
        Serial.println("Bulb set to yellow with smooth transition.");
    } else {
        Serial.println("Error setting bulb color.");
    }
    delay(5000); // Wait for 5 seconds

    // Set the bulb color to blue with sudden transition
    if (bulb.set_hsv_color(240, 100, EFFECT_SUDDEN) == ResponseType::SUCCESS) {
        Serial.println("Bulb set to blue with sudden transition.");
    } else {
        Serial.println("Error setting bulb color.");
    }
    delay(5000); // Wait for 5 seconds

    // Set the bulb color to purple with a low brightness
    if (bulb.set_hsv_color(270, 50, 25) == ResponseType::SUCCESS) {
        Serial.println("Bulb set to purple with smooth transition and low brightness.");
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