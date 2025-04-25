// Example: Timer Off Demo
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
    // Add a cron job to turn off the bulb after 30 minutes
    if (bulb.set_turn_off_delay(30) == ResponseType::SUCCESS) {
        Serial.println("Cron job added.");
    } else {
        Serial.println("Error adding cron job.");
    }
    delay(5000); // Wait for 5 seconds

    // Turn on the bulb
    if (bulb.turn_on() == ResponseType::SUCCESS) {
        Serial.println("Bulb turned on.");
    } else {
        Serial.println("Error turning on bulb.");
    }
    delay(5000); // Wait for 5 seconds

    // Remove the cron job
    if (bulb.remove_turn_off_delay() == ResponseType::SUCCESS) {
        Serial.println("Cron job removed.");
    } else {
        Serial.println("Error removing cron job.");
    }
    delay(5000); // Wait for 5 seconds
}