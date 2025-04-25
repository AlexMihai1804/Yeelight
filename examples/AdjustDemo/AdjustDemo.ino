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
    if (bulb.connect(bulbIP) == ResponseType::SUCCESS) {
        Serial.println("✔ Connected to Yeelight bulb");
    } else {
        Serial.println("✖ Connection failed");
        while (true) delay(1000);
    }
}

void loop() {
    if (bulb.adjust_brightness(20) == ResponseType::SUCCESS)
        Serial.println("Brightness +20%");
    else
        Serial.println("Error adjusting brightness");
    delay(3000);

    if (bulb.adjust_color_temp(-10) == ResponseType::SUCCESS)
        Serial.println("Color temp -10%");
    else
        Serial.println("Error adjusting color temperature");
    delay(3000);

    if (bulb.adjust_color(30) == ResponseType::SUCCESS)
        Serial.println("Color shift +30%");
    else
        Serial.println("Error adjusting color");
    delay(5000);
}
