// Example: Blink Demo
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
    } else {
        Serial.println("Connected to Yeelight bulb.");
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