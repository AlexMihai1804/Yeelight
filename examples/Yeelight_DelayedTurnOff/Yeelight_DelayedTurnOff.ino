// Example: Delayed Turn‑Off Demo
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
    // Set an auto delay off of 10 minutes
    if (bulb.set_scene_auto_delay_off(50, 600000) == ResponseType::SUCCESS) {
        Serial.println("Bulb will turn off automatically after 10 minutes.");
    } else {
        Serial.println("Error setting auto delay off.");
    }

    // Turn on the bulb
    if (bulb.turn_on() == ResponseType::SUCCESS) {
        Serial.println("Bulb turned on.");
    } else {
        Serial.println("Error turning on bulb.");
    }
}