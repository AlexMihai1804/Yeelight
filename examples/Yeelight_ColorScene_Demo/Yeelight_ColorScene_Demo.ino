#include <Yeelight.h>
#include <WiFi.h>

#define WIFI_SSID "YourWiFiSSID"
#define WIFI_PASS "YourWiFiPassword"
const uint8_t bulbIP[] = {192,168,1,100};

Yeelight bulb;

void setup() {
    Serial.begin(115200);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
    Serial.println("\nWiFi connected");
    if (bulb.connect(bulbIP) != ResponseType::SUCCESS) {
        Serial.println("Connect error"); while(true) delay(1000);
    }
    Serial.println("Connected to Yeelight");
}

void loop() {
    if (bulb.set_scene_rgb(0, 0, 255, 70) == ResponseType::SUCCESS)
        Serial.println("RGB scene set");
    else
        Serial.println("Error setting RGB scene");
    delay(5000);

    if (bulb.set_scene_hsv(60, 100, 80) == ResponseType::SUCCESS)
        Serial.println("HSV scene set");
    else
        Serial.println("Error setting HSV scene");
    delay(5000);

    if (bulb.set_scene_color_temperature(3000, 60) == ResponseType::SUCCESS)
        Serial.println("CT scene set");
    else
        Serial.println("Error setting CT scene");
    delay(5000);
}
