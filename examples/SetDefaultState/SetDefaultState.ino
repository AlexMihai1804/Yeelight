#include <WiFi.h>
#include <Yeelight.h>

#define WIFI_SSID "YourWiFiSSID"
#define WIFI_PASS "YourWiFiPassword"
const uint8_t bulbIP[] = {192, 168, 1, 100};

Yeelight bulb;

void setup() {
    Serial.begin(115200);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED) {
        delay(200);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected");
    if (bulb.connect(bulbIP) != ResponseType::SUCCESS) {
        Serial.println("✖ Connection failed");
        while (true) delay(1000);
    }
    if (bulb.set_default_state() == ResponseType::SUCCESS)
        Serial.println("✔ Default state saved");
    else
        Serial.println("✖ Error saving default state");
}

void loop() {
    // nothing
}
