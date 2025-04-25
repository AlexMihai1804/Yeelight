// Example: Connection Check Demo
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
    // Check if the bulb is connected
    if (bulb.is_connected()) {
        // Try to set an invalid color
        if (bulb.set_rgb_color(256, 0, 0) != ResponseType::SUCCESS) { // Invalid red value (256)
            Serial.println("Error setting bulb color.");
        } else {
            Serial.println("Bulb color set.");
        }
    } else {
        Serial.println("Bulb is not connected.");
    }
    delay(5000); // Wait for 5 seconds
}