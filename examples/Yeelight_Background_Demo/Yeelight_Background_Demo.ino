#include <Yeelight.h>
#include <WiFi.h>

#define WIFI_SSID "YourWiFiSSID"
#define WIFI_PASS "YourWiFiPassword"
const uint8_t bulbIP[] = {192,168,1,100};

Yeelight bulb;

void setup() {
    Serial.begin(115200);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (WiFi.status()!=WL_CONNECTED) { delay(500); Serial.print("."); }
    if (bulb.connect(bulbIP)!=ResponseType::SUCCESS) {
        Serial.println("Connect error"); while(true) delay(1000);
    }
    // pornire background
    bulb.set_power(true, MODE_CURRENT, BACKGROUND_LIGHT);
    delay(1000);
    // setare luminozitate background 30%
    bulb.set_brightness(30, BACKGROUND_LIGHT);
    delay(1000);
    // setare culoare background albastru
    bulb.set_rgb_color(0,0,255, BACKGROUND_LIGHT);
    delay(1000);
    // toggle background
    bulb.toggle_power(BACKGROUND_LIGHT);
    delay(1000);
    // oprire background
    bulb.turn_off(BACKGROUND_LIGHT);
}

void loop() {
    // nu facem altceva
}
