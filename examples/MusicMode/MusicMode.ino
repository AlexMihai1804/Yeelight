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
    if (bulb.enable_music_mode() == ResponseType::SUCCESS) {
        Serial.println("✔ Music mode ON");
        for (int i = 0; i < 100; ++i) {
            uint8_t r = (i * 2) & 0xFF;
            uint8_t g = 255 - r;
            uint8_t b = 128;
            bulb.set_rgb_color(r, g, b);
            delay(50);
        }
        bulb.disable_music_mode();
        Serial.println("✔ Music mode OFF");
    } else {
        Serial.println("✖ Error enabling music mode");
    }
}

void loop() {
    // nothing
}
