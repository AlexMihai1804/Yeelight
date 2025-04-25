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
    if (bulb.set_scene_rgb(128,0,128,70, BACKGROUND_LIGHT) == ResponseType::SUCCESS)
        Serial.println("Background RGB scene");
    else
        Serial.println("Error BG RGB");
    delay(5000);

    if (bulb.set_scene_hsv(90,100,50, BACKGROUND_LIGHT) == ResponseType::SUCCESS)
        Serial.println("Background HSV scene");
    else
        Serial.println("Error BG HSV");
    delay(5000);

    if (bulb.set_scene_color_temperature(3000,40, BACKGROUND_LIGHT) == ResponseType::SUCCESS)
        Serial.println("Background CT scene");
    else
        Serial.println("Error BG CT");
    delay(5000);
}
