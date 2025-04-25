#include <Yeelight.h>
#include <WiFi.h>
#include <FlowDefault.h>

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
    Flow disco = FlowDefault::disco(100);
    if (bulb.start_flow(disco) == ResponseType::SUCCESS)
        Serial.println("Disco flow started");
    else
        Serial.println("Error starting disco");
}

void loop() {
    // nothing here, flow runs until stopped
}