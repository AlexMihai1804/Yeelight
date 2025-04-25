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
    if (bulb.toggle_power(MAIN_LIGHT) == ResponseType::SUCCESS) Serial.println("Main toggled");
    else Serial.println("Error toggling main");
    delay(2000);

    if (bulb.toggle_power(BACKGROUND_LIGHT) == ResponseType::SUCCESS) Serial.println("Background toggled");
    else Serial.println("Error toggling background");
    delay(2000);

    if (bulb.toggle_power(BOTH) == ResponseType::SUCCESS) Serial.println("Both toggled");
    else Serial.println("Error toggling both");
    delay(5000);
}
